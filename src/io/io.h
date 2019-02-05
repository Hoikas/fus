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

#ifndef __FUS_IO_H
#define __FUS_IO_H

#include <openssl/ossl_typ.h>
#include <string_theory/string>
#include <tuple>
#include <uv.h>

namespace fus
{
    struct io_crypt_bn_t
    {
        BN_CTX* ctx;
    };

    void io_init();
    void io_close();

    bool str2addr(const char*, uint16_t, sockaddr_storage*);

    std::tuple<ST::string, ST::string, ST::string> io_generate_keys(uint32_t g_value);

    template <size_t _KSz, size_t _NSz, size_t _XSz>
    void io_generate_keys(uint32_t g_value, uint8_t (&k_key)[_KSz], uint8_t (&n_key)[_NSz], uint8_t (&x_key)[_XSz]);
};

#endif
