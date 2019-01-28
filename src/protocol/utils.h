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

#ifndef __FUS_PROTOCOL_UTILS_H
#define __FUS_PROTOCOL_UTILS_H

#include "core/uuid.h"
#include "io/net_struct.h"
#include <string_theory/string>

namespace fus
{
    namespace protocol
    {
        namespace _fields
        {
            template <typename _T, size_t _Sz>
            constexpr size_t size(_T(&)[_Sz]) { return _Sz; }
        }

#pragma pack(push,1)
        template<typename _Header, typename _Msg>
        struct net_msg
        {
            _Header m_header;
            _Msg m_contents;

            operator void*() { return (void*)(&m_header); }
        };
#pragma pack(pop)
    }
};

#endif
