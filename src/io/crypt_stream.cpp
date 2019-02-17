/*   This file is part of fus.
 *
 *   fus is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   fus is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with fus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string_theory/st_codecs.h>

#include "core/errors.h"
#include "crypt_stream.h"
#include "fus_config.h"
#include "io.h"
#include "net_struct.h"

// =================================================================================

// Reduces the allocations
namespace fus
{
    extern io_crypt_bn_t io_crypt_bn;
    extern size_t io_crypt_bufsz;
    extern void* io_crypt_buf;
};

// Manually defining this message allows us to avoid a circular link with fus_protocol
static fus::net_field_t s_cryptHandshakeFields[] = {
    { fus::net_field_t::data_type::e_integer, "type", 1 },
    { fus::net_field_t::data_type::e_integer, "msgsz", 1 },
};

static fus::net_struct_t s_cryptHandshakeStruct{ "crypt_handhake", 2, s_cryptHandshakeFields };

// Message IDs
enum
{
    e_connect,
    e_encrypt,
};

// =================================================================================

static inline void* _realloc_buffer(size_t bufsz)
{
    if (bufsz > fus::io_crypt_bufsz) {
        size_t blocks = bufsz / sizeof(size_t);
        fus::io_crypt_bufsz = (blocks + 16) * sizeof(size_t);
        /// fixme: potential leak if realloc() returns nullptr
        fus::io_crypt_buf = realloc(fus::io_crypt_buf, fus::io_crypt_bufsz);
    }
    return fus::io_crypt_buf;
}

// =================================================================================

void fus::crypt_stream_init(fus::crypt_stream_t* stream)
{
    stream->m_encryptcb = nullptr;
#ifndef FUS_ALLOW_DECRYPTED_CLIENTS
    ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_mustEncrypt;
#endif
}

void fus::crypt_stream_free(fus::crypt_stream_t* stream)
{
    tcp_stream_t* tcp = (tcp_stream_t*)stream;
    if (tcp->m_flags & tcp_stream_t::e_encrypted) {
        EVP_CIPHER_CTX_free(stream->m_crypt.encrypt);
        EVP_CIPHER_CTX_free(stream->m_crypt.decrypt);
        tcp->m_flags &= ~tcp_stream_t::e_encrypted;
    }
    crypt_stream_free_keys(stream);
}

void fus::crypt_stream_free_keys(fus::crypt_stream_t* stream)
{
    tcp_stream_t* tcp = (tcp_stream_t*)stream;
    if (tcp->m_flags & tcp_stream_t::e_hasCliKeys) {
        if (tcp->m_flags & tcp_stream_t::e_ownKeys) {
            BN_free(stream->m_crypt.n);
            BN_free(stream->m_crypt.x);
            tcp->m_flags &= ~tcp_stream_t::e_ownKeys;
        }
        if (tcp->m_flags & tcp_stream_t::e_ownSeed) {
            BN_free(stream->m_crypt.seed);
            tcp->m_flags &= ~tcp_stream_t::e_ownSeed;
        }
        tcp->m_flags &= ~tcp_stream_t::e_hasCliKeys;
    }
    if (tcp->m_flags & tcp_stream_t::e_hasSrvKeys) {
        if (tcp->m_flags & tcp_stream_t::e_ownKeys) {
            BN_free(stream->m_crypt.k);
            BN_free(stream->m_crypt.n);
            tcp->m_flags &= ~tcp_stream_t::e_ownKeys;
        }
        tcp->m_flags &= ~tcp_stream_t::e_hasSrvKeys;
    }
}

// =================================================================================

void fus::crypt_stream_set_keys_client(fus::crypt_stream_t* stream, uint32_t g, BIGNUM* n, BIGNUM* x)
{
    FUS_ASSERTD(!(((tcp_stream_t*)stream)->m_flags & tcp_stream_t::e_encrypted));
    ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_hasCliKeys;

    stream->m_crypt.g = g;
    stream->m_crypt.n = n;
    stream->m_crypt.x = x;
}

static inline void _load_key(BIGNUM* bn, const ST::string& key)
{
    uint8_t buf[64];
    FUS_ASSERTD(ST::base64_decode(key, buf, sizeof(buf)) == sizeof(buf));
    BN_bin2bn((unsigned char*)buf, sizeof(buf), bn);
}

void fus::crypt_stream_set_keys_client(fus::crypt_stream_t* stream, uint32_t g, const ST::string& n, const ST::string& x)
{
    BIGNUM* nkey = BN_new();
    BIGNUM* xkey = BN_new();

    _load_key(nkey, n);
    _load_key(xkey, x);

    crypt_stream_set_keys_client(stream, g, nkey, xkey);
    ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_ownKeys;
}

void fus::crypt_stream_set_keys_server(fus::crypt_stream_t* stream, BIGNUM* k, BIGNUM* n)
{
    FUS_ASSERTD(!(((tcp_stream_t*)stream)->m_flags & tcp_stream_t::e_encrypted));
    ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_hasSrvKeys;

    stream->m_crypt.k = k;
    stream->m_crypt.n = n;
}

// =================================================================================

void fus::crypt_stream_must_encrypt(fus::crypt_stream_t* stream, bool value)
{
    if (value)
        ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_mustEncrypt;
    else
        ((tcp_stream_t*)stream)->m_flags &= ~tcp_stream_t::e_mustEncrypt;
}

// =================================================================================

static EVP_CIPHER_CTX* _init_evp(const uint8_t* key, size_t keylen, int enc)
{
    const EVP_CIPHER* cipher;
    if (key == nullptr)
        cipher = EVP_enc_null();
    else
        cipher = EVP_rc4();

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_CipherInit_ex(ctx, cipher, nullptr, nullptr, nullptr, enc);

    // I am uncertain as to whether this is a bug in OpenSSL or just a piss-poor design decision.
    // RC4 supposedly allows "variable key lengths" in EVP. OK, but when you call CipherInit_ex,
    // it uses the key length it already has (16 bytes). Calling EVP_CIPHER_CTX_set_key_length does
    // not refresh the key. Trying to use EVP_CIPHER_CTX_set_key_length before CipherInit does not
    // work because then the cipher is NULL. Trying to reinit the cipher causes the custom key
    // length to be discarded... UGHHHH
    // So, we will set the custom key length and manually pump the cipher init function. Notice that
    // we didn't provide the key in the call to CipherInit_ex? That avoided part of the init proc.
    if (key) {
        EVP_CIPHER_CTX_set_key_length(ctx, keylen);
        auto initproc = EVP_CIPHER_meth_get_init(cipher);
        FUS_ASSERTD(initproc(ctx, key, nullptr, enc) == 1);
    }

    // OpenSSL needs to die in a fire.
    return ctx;
}

static void _init_encryption(fus::crypt_stream_t* stream, const uint8_t* seed, const uint8_t* key, size_t keylen)
{
    // write encryption reply
    size_t msgsz = 2 + keylen;
    uint8_t* reply = (uint8_t*)alloca(msgsz);
    {
        uint8_t* ptr = reply;
        *ptr++ = e_encrypt;
        *ptr++ = msgsz;
        if (seed)
            memcpy(ptr, seed, keylen);
    }
    // not waiting for the write to finish, no more decrypted messages are allowed.
    fus::tcp_stream_write((fus::tcp_stream_t*)stream, reply, msgsz);

    stream->m_crypt.encrypt = _init_evp(key, keylen, 1);
    stream->m_crypt.decrypt = _init_evp(key, keylen, 0);
    stream->m_stream.m_flags |= fus::tcp_stream_t::e_encrypted;
    if (stream->m_encryptcb)
        stream->m_encryptcb(stream, 0);
}

static void _handshake_ydata_read(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* buf)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)stream);
        return;
    }

    // Hey, nice, OpenSSL has considered that we'll want temporary bignums...
    BN_CTX_start(fus::io_crypt_bn.ctx);
    BIGNUM* y = BN_CTX_get(fus::io_crypt_bn.ctx);
    BIGNUM* seed = BN_CTX_get(fus::io_crypt_bn.ctx);

    BN_lebin2bn((unsigned char*)buf, (int)nread, y);
    BN_mod_exp(seed, y, stream->m_crypt.k, stream->m_crypt.n, fus::io_crypt_bn.ctx);

    uint8_t cli_seed[64];
    uint8_t srv_seed[7];
    BN_bn2lebinpad(seed, cli_seed, sizeof(cli_seed));
    BN_CTX_end(fus::io_crypt_bn.ctx);
    fus::crypt_stream_free_keys(stream);

    RAND_bytes(srv_seed, sizeof(srv_seed));
    uint8_t key[7];
    for (size_t i = 0; i < sizeof(key); ++i)
        key[i] = cli_seed[i] ^ srv_seed[i];
    _init_encryption(stream, srv_seed, key, sizeof(key));
}

static void _handshake_header_read_srv(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* msg)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)stream);
        return;
    }

    uint8_t msg_id = *msg;
    uint8_t msgsz = *(msg+1);

    // A client will send no Y data if encryption is not desired.
    uint8_t ybufsz = msgsz - 2;
    if (ybufsz == 0) {
        if (((fus::tcp_stream_t*)stream)->m_flags & fus::tcp_stream_t::e_mustEncrypt)
            fus::tcp_stream_shutdown((fus::tcp_stream_t*)stream);
        else
            _init_encryption(stream, nullptr, nullptr, 0);
    } else {
        // Read in the Y-Data from the client
        fus::tcp_stream_read((fus::tcp_stream_t*)stream, ybufsz, (fus::tcp_read_cb)_handshake_ydata_read);
    }
}

void fus::crypt_stream_establish_server(fus::crypt_stream_t* stream, crypt_established_cb cb)
{
    FUS_ASSERTD(((tcp_stream_t*)stream)->m_flags & tcp_stream_t::e_hasSrvKeys);
    stream->m_encryptcb = cb;
    tcp_stream_read_struct((tcp_stream_t*)stream, &s_cryptHandshakeStruct, (tcp_read_cb)_handshake_header_read_srv);
}

// =================================================================================

static void _srvseed_read(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* srv_seed)
{
    if (nread < 0) {
        if (stream->m_encryptcb)
            stream->m_encryptcb(stream, nread);
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)stream);
        return;
    }

    uint8_t cli_seed[64];
    BN_bn2lebinpad(stream->m_crypt.seed, cli_seed, sizeof(cli_seed));
    fus::crypt_stream_free_keys(stream);

    uint8_t* key = (uint8_t*)alloca(nread);
    for (ssize_t i = 0; i < nread; ++i) {
        key[i] = cli_seed[i] ^ srv_seed[i];
    }
    stream->m_crypt.encrypt = _init_evp(key, nread, 1);
    stream->m_crypt.decrypt = _init_evp(key, nread, 0);
    ((fus::tcp_stream_t*)stream)->m_flags |= fus::tcp_stream_t::e_encrypted;

    if (stream->m_encryptcb)
        stream->m_encryptcb(stream, 0);
}

static void _handshake_header_read_cli(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* msg)
{
    if (nread < 0 || msg[0] != e_encrypt || msg[1] < 9 || msg[1] > 4096) {
        if (stream->m_encryptcb)
            stream->m_encryptcb(stream, nread < 0 ? nread : UV_EMSGSIZE);
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)stream);
        return;
    }

    uint8_t bufsz = msg[1] - 2;
    fus::tcp_stream_read((fus::tcp_stream_t*)stream, bufsz, (fus::tcp_read_cb)_srvseed_read);
}

void fus::crypt_stream_establish_client(fus::crypt_stream_t* stream, crypt_established_cb cb)
{
    FUS_ASSERTD(((tcp_stream_t*)stream)->m_flags & tcp_stream_t::e_hasCliKeys);
    stream->m_encryptcb = cb;

    // What a turd. We're going to need to keep this bignum for a bit.
    stream->m_crypt.seed = BN_new();
    ((tcp_stream_t*)stream)->m_flags |= tcp_stream_t::e_ownSeed;

    BN_CTX_start(fus::io_crypt_bn.ctx);
    BIGNUM* b = BN_CTX_get(fus::io_crypt_bn.ctx);
    BIGNUM* y = BN_CTX_get(fus::io_crypt_bn.ctx);
    BIGNUM* g = BN_CTX_get(fus::io_crypt_bn.ctx);

    // Values taken from CWE's plBigNum::Rand(), used by NetMsgCryptClientStart()
    BN_rand(b, 512, BN_RAND_TOP_ONE, BN_RAND_BOTTOM_ANY);
    BN_set_word(g, stream->m_crypt.g);

    // This is easier to follow in the old timey pyfus. Maybe one day, its code will be resurrected...
    BN_mod_exp(stream->m_crypt.seed, stream->m_crypt.x, b, stream->m_crypt.n, fus::io_crypt_bn.ctx);
    BN_mod_exp(y, g, b, stream->m_crypt.n, fus::io_crypt_bn.ctx);

    // Send the Y-data to the server and await its crypt response.
    uint8_t connect[66];
    {
        uint8_t* ptr = connect;
        *ptr++ = e_connect;
        *ptr++ = sizeof(connect);
        BN_bn2lebinpad(y, ptr, sizeof(connect) - 2);
    }
    tcp_stream_write((tcp_stream_t*)stream, connect, sizeof(connect));
    tcp_stream_read_struct((tcp_stream_t*)stream, &s_cryptHandshakeStruct, (tcp_read_cb)_handshake_header_read_cli);

    // Nukes the temp bignums
    BN_CTX_end(fus::io_crypt_bn.ctx);
}

// =================================================================================

static inline void _cipher(EVP_CIPHER_CTX* ctx, void* msg, size_t msgsz)
{
    unsigned char* outbuf;
    if (msgsz <= 4096)
        outbuf = (unsigned char*)alloca(msgsz);
    else
        outbuf = (unsigned char*)_realloc_buffer(msgsz);
    int encsize;
    EVP_CipherUpdate(ctx, outbuf, &encsize, (unsigned char*)msg, msgsz);
    FUS_ASSERTD(msgsz == encsize);
    memcpy(msg, outbuf, msgsz);
}

void fus::crypt_stream_decipher(fus::crypt_stream_t* stream, void* msg, size_t msgsz)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(msgsz);
    FUS_ASSERTD((stream->m_stream.m_flags & fus::tcp_stream_t::e_encrypted));
    _cipher(stream->m_crypt.decrypt, msg, msgsz);
}

void fus::crypt_stream_encipher(fus::crypt_stream_t* stream, void* msg, size_t msgsz)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(msg);
    FUS_ASSERTD(msgsz);
    FUS_ASSERTD((stream->m_stream.m_flags & fus::tcp_stream_t::e_encrypted));
    _cipher(stream->m_crypt.encrypt, msg, msgsz);
}
