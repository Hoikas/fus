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

#include "io.h"
#include "errors.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string_theory/st_codecs.h>

namespace fus
{
    io_crypt_bn_t io_crypt_bn{ nullptr, nullptr, nullptr, nullptr, nullptr };
    size_t io_crypt_bufsz = 0;
    void* io_crypt_buf = nullptr;
};

// ============================================================================

void fus::io_init()
{
    FUS_ASSERTD(RAND_poll());

    // This (hopefully) avoids some allocations
    io_crypt_bn.ctx = BN_CTX_new();
    io_crypt_bn.g = BN_new();
    io_crypt_bn.k = BN_new();
    io_crypt_bn.n = BN_new();
    io_crypt_bn.x = BN_new();

    // OpenSSL 1.0 compatibility
    OpenSSL_add_all_algorithms();
}

void fus::io_close()
{
    BN_CTX_free(io_crypt_bn.ctx);
    BN_free(io_crypt_bn.g);
    BN_free(io_crypt_bn.k);
    BN_free(io_crypt_bn.n);
    BN_free(io_crypt_bn.x);

    // OpenSSL 1.0 compatibility
    EVP_cleanup();
}

// ============================================================================

template <size_t _KSz, size_t _NSz, size_t _XSz>
void fus::io_generate_keys(uint32_t g_value, uint8_t (&k_key)[_KSz], uint8_t (&n_key)[_NSz], uint8_t (&x_key)[_XSz])
{
    static_assert (_KSz == _NSz);
    static_assert (_NSz == _XSz);

    // Generate primes for public and private keys
    BN_generate_prime_ex(io_crypt_bn.k, _KSz * 8, 1, nullptr, nullptr, nullptr);
    BN_generate_prime_ex(io_crypt_bn.n, _NSz * 8, 1, nullptr, nullptr, nullptr);

    // x = g**k%n
    BN_set_word(io_crypt_bn.g, g_value);
    BN_mod_exp(io_crypt_bn.x, io_crypt_bn.g, io_crypt_bn.k, io_crypt_bn.n, io_crypt_bn.ctx);

    BN_bn2bin(io_crypt_bn.k, k_key);
    BN_bn2bin(io_crypt_bn.n, n_key);
    BN_bn2bin(io_crypt_bn.x, x_key);
}

std::tuple<ST::string, ST::string, ST::string> fus::io_generate_keys(uint32_t g_value)
{
    uint8_t k_key[64];
    uint8_t n_key[64];
    uint8_t x_key[64];
    io_generate_keys(g_value, k_key, n_key, x_key);
    return std::make_tuple(ST::base64_encode(k_key, 64),
                           ST::base64_encode(n_key, 64),
                           ST::base64_encode(x_key, 64));
}
