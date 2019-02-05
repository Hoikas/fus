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

#include "core/errors.h"
#include "io.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string_theory/st_codecs.h>

namespace fus
{
    io_crypt_bn_t io_crypt_bn{ nullptr };
    size_t io_crypt_bufsz = 0;
    void* io_crypt_buf = nullptr;
};

// ============================================================================

void fus::io_init()
{
    FUS_ASSERTD(RAND_poll());

    // This (hopefully) avoids some allocations
    io_crypt_bn.ctx = BN_CTX_new();

    // OpenSSL 1.0 compatibility
    OpenSSL_add_all_algorithms();
}

void fus::io_close()
{
    BN_CTX_free(io_crypt_bn.ctx);

    // OpenSSL 1.0 compatibility
    EVP_cleanup();
}

// ============================================================================

bool fus::str2addr(const char* str, uint16_t port, sockaddr_storage* addr)
{
    int ipv4result = uv_ip4_addr(str, port, (sockaddr_in*)addr);
    if (ipv4result < 0) {
        int ipv6result = uv_ip6_addr(str, port, (sockaddr_in6*)addr);
        return ipv6result >= 0;
    } else {
        return ipv4result >= 0;
    }
}

// ============================================================================

template <size_t _KSz, size_t _NSz, size_t _XSz>
void fus::io_generate_keys(uint32_t g_value, uint8_t (&k_key)[_KSz], uint8_t (&n_key)[_NSz], uint8_t (&x_key)[_XSz])
{
    static_assert (_KSz == _NSz);
    static_assert (_NSz == _XSz);

    // Keys should be 512 bits
    constexpr int key_bits = _KSz * 8;
    static_assert(key_bits == 512);

    // According to OpenSSL's documentation, this works well for tight loops
    BN_CTX_start(io_crypt_bn.ctx);
    BIGNUM* g = BN_CTX_get(io_crypt_bn.ctx);
    BIGNUM* k = BN_CTX_get(io_crypt_bn.ctx);
    BIGNUM* n = BN_CTX_get(io_crypt_bn.ctx);
    BIGNUM* x = BN_CTX_get(io_crypt_bn.ctx);

    // Generate primes for public (N) and private (K/A) keys
    BN_generate_prime_ex(k, key_bits, 1, nullptr, nullptr, nullptr);
    BN_generate_prime_ex(n, key_bits, 1, nullptr, nullptr, nullptr);

    // Compute the client key (N/KA)
    // X = g**K%N
    BN_set_word(g, g_value);
    BN_mod_exp(x, g, k, n, io_crypt_bn.ctx);

    // Store the keys
    BN_bn2bin(k, k_key);
    BN_bn2bin(n, n_key);
    BN_bn2bin(x, x_key);

    // Releases the temporary bignums
    BN_CTX_end(io_crypt_bn.ctx);
}

std::tuple<ST::string, ST::string, ST::string> fus::io_generate_keys(uint32_t g_value)
{
    uint8_t k_key[64];
    uint8_t n_key[64];
    uint8_t x_key[64];
    io_generate_keys(g_value, k_key, n_key, x_key);
    return std::make_tuple(ST::base64_encode(k_key, sizeof(k_key)),
                           ST::base64_encode(n_key, sizeof(n_key)),
                           ST::base64_encode(x_key, sizeof(x_key)));
}
