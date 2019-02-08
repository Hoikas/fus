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

#ifndef __FUS_CRYPT_STREAM
#define __FUS_CRYPT_STREAM

#include <openssl/ossl_typ.h>

#include "tcp_stream.h"

namespace fus
{
    struct crypt_stream_t;
    typedef void (*crypt_established_cb)(crypt_stream_t*, ssize_t);

    struct crypt_stream_t
    {
        tcp_stream_t m_stream;
        tcp_read_cb m_readcb;
        crypt_established_cb m_encryptcb;
        union
        {
            struct
            {
                uint32_t g;
                BIGNUM* n;
                BIGNUM* seed;
                BIGNUM* x;
                BIGNUM* k;
            };
            struct
            {
                EVP_CIPHER_CTX* encrypt;
                EVP_CIPHER_CTX* decrypt;
            };
        } m_crypt;
    };

    void crypt_stream_init(crypt_stream_t*);
    void crypt_stream_free(crypt_stream_t*);
    void crypt_stream_free_keys(crypt_stream_t*);

    void crypt_stream_set_keys_client(crypt_stream_t*, uint32_t, BIGNUM*, BIGNUM*);
    void crypt_stream_set_keys_client(crypt_stream_t*, uint32_t, const ST::string&, const ST::string&);
    void crypt_stream_set_keys_server(crypt_stream_t*, BIGNUM*, BIGNUM*);

    void crypt_stream_establish_server(crypt_stream_t*, crypt_established_cb cb=nullptr);
    void crypt_stream_establish_client(crypt_stream_t*, crypt_established_cb cb=nullptr);

    void crypt_stream_decipher(crypt_stream_t*, void*, size_t);
    void crypt_stream_encipher(crypt_stream_t*, void*, size_t);
};

#endif
