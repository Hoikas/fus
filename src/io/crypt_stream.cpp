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

#include "crypt_stream.h"
#include "errors.h"
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

void fus::crypt_stream_close(fus::crypt_stream_t* stream, uv_close_cb cb)
{
    if (stream->m_stream.m_flags & tcp_stream_t::e_encrypted) {
        EVP_CIPHER_CTX_free(stream->m_crypt.encrypt);
        EVP_CIPHER_CTX_free(stream->m_crypt.decrypt);
    }
    tcp_stream_close((tcp_stream_t*)stream, cb);
}

// =================================================================================

static void _handshake_ydata_read(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* buf)
{
    if (nread < 0) {
        fus::crypt_stream_close(stream);
        return;
    }

    // The purpose of this cryptography is not that we're trying to hide nuclear secrets. Rather,
    // we are simply trying to authenticate the client and server with one another. If any data leaks
    // out of here, allowing connection snooping... Well, who cares. This is URU.
    BN_lebin2bn(buf, (int)nread, fus::io_crypt_bn.x);
    BN_mod_exp(fus::io_crypt_bn.g, fus::io_crypt_bn.x, stream->m_crypt.k, stream->m_crypt.n, fus::io_crypt_bn.ctx);

    uint8_t cli_seed[64];
    uint8_t srv_seed[7];
    if (BN_bn2lebinpad(fus::io_crypt_bn.g, cli_seed, 64) < 0) {
        fus::crypt_stream_close(stream);
        return;
    }
    RAND_bytes(srv_seed, sizeof(srv_seed));
    uint8_t key[7];
    for (size_t i = 0; i < sizeof(key); ++i)
        key[i] = cli_seed[i] ^ srv_seed[i];
    stream->m_stream.m_flags |= fus::tcp_stream_t::e_encrypted;

    /// TODO: investigate... can we share the EVP context?
    // init send encryption
    stream->m_crypt.encrypt = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(stream->m_crypt.encrypt, EVP_rc4(), nullptr, key, nullptr);
    EVP_CIPHER_CTX_set_key_length(stream->m_crypt.encrypt, sizeof(key));

    // init recv encryption
    stream->m_crypt.decrypt = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(stream->m_crypt.decrypt, EVP_rc4(), nullptr, key, nullptr);
    EVP_CIPHER_CTX_set_key_length(stream->m_crypt.decrypt, sizeof(key));

    // write encryption reply
    uint8_t reply[9];
    {
        uint8_t* ptr = reply;
        *ptr++ = e_encrypt;
        *ptr++ = sizeof(reply);
        memcpy(ptr, key, sizeof(key));
    }
    // not waiting for the write to finish, no more decrypted messages are allowed.
    fus::tcp_stream_write((fus::tcp_stream_t*)stream, reply, sizeof(reply));
}

static void _handshake_header_read(fus::crypt_stream_t* stream, ssize_t nread, uint8_t* msg)
{
    if (nread < 0) {
        fus::crypt_stream_close(stream);
        return;
    }

    uint8_t msg_id = *msg;
    uint8_t msgsz = *(msg+1);

    // A client will send no Y data if encryption is not desired.
    uint8_t ybufsz = msgsz - 2;
    if (ybufsz == 0) {
        /// TODO: support this in an intelligent manner (namely, allow support for this to be
        ///       disabled somewhere. probably at the cmake level.
        fus::crypt_stream_close(stream);
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
    tcp_stream_read_msg((tcp_stream_t*)stream, &s_cryptHandshakeStruct, (tcp_read_cb)_handshake_header_read);
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
        EVP_DecryptUpdate(stream->m_crypt.decrypt, outbuf, &encsize, msg, nread);
        memcpy(msg, outbuf, nread);
    }

    fus::tcp_read_cb cb = nullptr;
    std::swap(cb, stream->m_readcb);
    cb((fus::tcp_stream_t*)stream, nread, msg);
}

void fus::crypt_stream_read_msg(fus::crypt_stream_t* stream, const struct fus::net_struct_t* ns, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(read_cb);

    stream->m_readcb = read_cb;
    tcp_stream_read_msg((tcp_stream_t*)stream, ns, (tcp_read_cb)_read_complete);
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
        EVP_EncryptUpdate(stream->m_crypt.encrypt, outbuf, &encsize, (unsigned char*)buf, bufsz);
        tcp_stream_write((tcp_stream_t*)stream, outbuf, bufsz, write_cb);
    } else {
        tcp_stream_write((tcp_stream_t*)stream, buf, bufsz, write_cb);
    }
}
