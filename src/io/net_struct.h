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

#ifndef __FUS_NET_STRUCT_H
#define __FUS_NET_STRUCT_H

#include <cstdint>
#include <iosfwd>

namespace fus
{
    struct net_field_t final
    {
        enum class data_type
        {
            /** Plain old int. */
            e_integer,

            /** Transaction ID data type... it's a plain old int */
            e_transaction,

            /** A fixed size buffer. */
            e_blob,

            /** A fixed size string buffer */
            e_string,

            /** An arbitrarily sized buffer */
            e_buffer,
        };

        data_type m_type;
        const char* m_name;
        size_t m_datasz;
    };

    struct net_struct_t final
    {
        const char* m_name;
        size_t m_size;
        const net_field_t* m_fields;
    };

    size_t net_struct_calcsz(const net_struct_t*);
    void net_struct_printf(const net_struct_t*, FILE*);
    void net_msg_printf(const net_struct_t*, const void*, FILE*);
};

#endif
