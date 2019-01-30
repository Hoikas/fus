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

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "core/errors.h"
#include "crypt_stream.h"
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
    stream->m_readcb = nullptr;
    stream->m_encryptcb = nullptr;
}

void fus::crypt_stream_free(fus::crypt_stream_t* stream)
{
    if (stream->m_stream.m_flags & tcp_stream_t::e_encrypted) {
        EVP_CIPHER_CTX_free(stream->m_crypt.encrypt);
        EVP_CIPHER_CTX_free(stream->m_crypt.decrypt);
    }
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
    stream->m_stream.m_flags |= fus::tcp_stream_t::e_encrypted;
    stream->m_crypt.encrypt = _init_evp(key, keylen, 1);
    stream->m_crypt.decrypt = _init_evp(key, keylen, 0);

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

    RAND_bytes(srv_seed, sizeof(srv_seed));
    uint8_t key[7];
    for (size_t i = 0; i < sizeof(key); ++i)
        key[i] = cli_seed[i] ^ srv_seed[i];
    _init_encryption(stream, srv_seed, key, sizeof(key));
}

static void _handshake_header_read(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* msg)
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
        /// TODO: allow this to be disabled in cmake
        _init_encryption(stream, nullptr, nullptr, 0);
    } else {
        // Read in the Y-Data from the client
        fus::tcp_stream_read((fus::tcp_stream_t*)stream, ybufsz, (fus::tcp_read_cb)_handshake_ydata_read);
    }
}

void fus::crypt_stream_establish_server(fus::crypt_stream_t* stream, BIGNUM* k, BIGNUM* n, crypt_established_cb cb)
{
    stream->m_crypt.k = k;
    stream->m_crypt.n = n;
    stream->m_encryptcb = cb;
    tcp_stream_read_struct((tcp_stream_t*)stream, &s_cryptHandshakeStruct, (tcp_read_cb)_handshake_header_read);
}

// =================================================================================

static void _read_complete(fus::crypt_stream_t* stream, ssize_t nread, unsigned char* msg)
{
    if (nread > 0 && stream->m_stream.m_flags & fus::tcp_stream_t::e_encrypted) {
        unsigned char* outbuf;
        if (nread <= 1024)
            outbuf = (unsigned char*)alloca(nread);
        else
            outbuf = (unsigned char*)_realloc_buffer(nread);
        int encsize;
        EVP_CipherUpdate(stream->m_crypt.decrypt, outbuf, &encsize, msg, nread);
        memcpy(msg, outbuf, nread);
    }

    fus::tcp_read_cb cb = nullptr;
    std::swap(cb, stream->m_readcb);
    cb((fus::tcp_stream_t*)stream, nread, msg);
}

void fus::crypt_stream_read_struct(fus::crypt_stream_t* stream, const struct fus::net_struct_t* ns, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(read_cb);

    stream->m_readcb = read_cb;
    tcp_stream_read_struct((tcp_stream_t*)stream, ns, (tcp_read_cb)_read_complete);
}

// =================================================================================

void fus::crypt_stream_write(fus::crypt_stream_t* stream, const void* buf, size_t bufsz, fus::tcp_write_cb write_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    if (stream->m_stream.m_flags & tcp_stream_t::e_encrypted) {
        unsigned char* outbuf;
        if (bufsz <= 1024)
            outbuf = (unsigned char*)alloca(bufsz);
        else
            outbuf = (unsigned char*)_realloc_buffer(bufsz);
        int encsize;
        EVP_CipherUpdate(stream->m_crypt.encrypt, outbuf, &encsize, (unsigned char*)buf, bufsz);
        tcp_stream_write((tcp_stream_t*)stream, outbuf, bufsz, write_cb);
    } else {
        tcp_stream_write((tcp_stream_t*)stream, buf, bufsz, write_cb);
    }
}
