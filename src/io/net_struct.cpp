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

#include "net_struct.h"

#include <cstdio>
#include "errors.h"

size_t fus::net_struct_calcsz(const net_struct_t* msg)
{
    size_t size = 0;
    for (size_t i = 0; i < msg->m_size; ++i)
        size += msg->m_fields[i].m_datasz;
    return size;
}

static const char* _get_data_type_str(fus::net_field_t::data_type type)
{
    switch (type) {
    case fus::net_field_t::data_type::e_blob:
        return "BLOB";
    case fus::net_field_t::data_type::e_buffer:
        return "BUFFER";
    case fus::net_field_t::data_type::e_integer:
        return "INTEGER";
    case fus::net_field_t::data_type::e_string:
        return "STRING";
    case fus::net_field_t::data_type::e_transaction:
        return "TRANSID";
    default:
        return "???";
    }
}

void fus::net_struct_printf(const net_struct_t* msg, FILE* f)
{
    fprintf(f, "--- BEGIN NETSTRUCT ---\n");
    fprintf(f, "    Name: '%s'\n", msg->m_name);
    fprintf(f, "    Field(s): %d\n", msg->m_size);
    for (size_t i = 0; i < msg->m_size; ++i) {
        const char* type = _get_data_type_str(msg->m_fields[i].m_type);
        fprintf(f, "    -> %s [TYPE: '%s'] [SIZE: %d]\n", msg->m_fields[i].m_name, type, msg->m_fields[i].m_datasz);
    }
    fprintf(f, "--- END   NETSTRUCT ---\n");
}

void fus::net_msg_printf(const net_struct_t* msg, const void* data, FILE* f)
{
    fprintf(f, "--- BEGIN NETMSG ---\n");
    fprintf(f, "    Name: '%s'\n", msg->m_name);
    fprintf(f, "    Field(s): %d\n", msg->m_size);

    size_t offset = 0;
    for (size_t i = 0; i < msg->m_size; ++i) {
        fprintf(f, "    -> %s\n", msg->m_fields[i].m_name);
        fprintf(f, "        - TYPE: '%s'\n", _get_data_type_str(msg->m_fields[i].m_type));
        fprintf(f, "        - SIZE: %d\n", msg->m_fields[i].m_datasz);

        uint8_t* datap = (uint8_t*)data + offset;
        switch (msg->m_fields[i].m_type) {
        case net_field_t::data_type::e_integer:
        case net_field_t::data_type::e_transaction:
            {
                fprintf(f, "        - DATA: ");
                /// fixme big endian
                switch (msg->m_fields[i].m_datasz) {
                case 1:
                    fprintf(f, "%hhu", *datap);
                    break;
                case 2:
                    fprintf(f, "%hu", *(uint16_t*)datap);
                    break;
                case 4:
                    fprintf(f, "%lu", *(uint32_t*)datap);
                    break;
                case 8:
                    fprintf(f, "%llu", *(uint64_t*)datap);
                    break;
                default:
                    FUS_ASSERTD(0);
                    break;
                }
            }
            fprintf(f, "\n");
            break;
        case net_field_t::data_type::e_string:
            fprintf(f, "        - DATA: '%S'\n", (char16_t*)datap);
            break;
        }

        offset += msg->m_fields[i].m_datasz;
    }

    fprintf(f, "--- END   NETMSG ---\n");
}
