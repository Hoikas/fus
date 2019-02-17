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

#ifndef __FUS_ENDIAN_H
#define __FUS_ENDIAN_H

#include <cstdint>
#include "fus_config.h"
#ifdef _MSC_VER
#   include <intrin.h>
#endif

namespace fus
{
    inline uint16_t swap_endian(uint16_t value)
    {
#ifdef _MSC_VER
        return _byteswap_ushort(value);
#else
        return __builtin_bswap16(value);
#endif
    }

    inline uint32_t swap_endian(uint32_t value)
    {
#ifdef _MSC_VER
        return _byteswap_ulong(value);
#else
        return __builtin_bswap32(value);
#endif
    }

    inline uint64_t swap_endian(uint64_t value)
    {
#ifdef _MSC_VER
        return _byteswap_uint64(value);
#else
        return __builtin_bswap64(value);
#endif
    }
};

#ifdef FUS_BIG_ENDIAN
#   define FUS_BE16(x) x
#   define FUS_BE32(x) x
#   define FUS_BE64(x) x
#   define FUS_LE16(x) fus::swap_endian(x);
#   define FUS_LE32(x) fus::swap_endian(x);
#   define FUS_LE64(x) fus::swap_endian(x);
#else
#   define FUS_BE16(x) fus::swap_endian(x);
#   define FUS_BE32(x) fus::swap_endian(x);
#   define FUS_BE64(x) fus::swap_endian(x);
#   define FUS_LE16(x) x
#   define FUS_LE32(x) x
#   define FUS_LE64(x) x
#endif

#endif
