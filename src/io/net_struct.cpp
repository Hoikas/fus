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
#include "errors.h"

#include <iostream>

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

void fus::net_struct_print(const net_struct_t* msg, std::ostream& stream)
{
    stream << "--- BEGIN NETSTRUCT ---" << std::endl;
    stream << "    Name: '" << msg->m_name << "'" << std::endl;
    stream << "    Field(s): " << msg->m_size << std::endl;
    for (size_t i = 0; i < msg->m_size; ++i) {
        const char* type = _get_data_type_str(msg->m_fields[i].m_type);
        stream << "    -> " << msg->m_fields[i].m_name << " ";
        stream << "[TYPE: '" << _get_data_type_str(msg->m_fields[i].m_type) << "'] ";
        stream << "[SIZE: " << msg->m_fields[i].m_datasz << "]" << std::endl;
    }
    stream << "--- END   NETSTRUCT ---" << std::endl;
}

void fus::net_msg_print(const net_struct_t* msg, const void* data, std::ostream& stream)
{
    stream << "--- BEGIN NETMSG ---" << std::endl;
    stream << "    Name: '" << msg->m_name << "'" << std::endl;
    stream << "    Field(s): " << msg->m_size << std::endl;

    size_t offset = 0;
    for (size_t i = 0; i < msg->m_size; ++i) {
        stream << "    -> " << msg->m_fields[i].m_name << std::endl;
        stream << "        - TYPE: '" << _get_data_type_str(msg->m_fields[i].m_type) << "'" << std::endl;
        stream << "        - SIZE: " << msg->m_fields[i].m_datasz << std::endl;

        uint8_t* datap = (uint8_t*)data + offset;
        switch (msg->m_fields[i].m_type) {
        case net_field_t::data_type::e_integer:
        case net_field_t::data_type::e_transaction:
            {
                stream << "        - DATA: ";
                /// fixme big endian
                switch (msg->m_fields[i].m_datasz) {
                case 1:
                    stream << (int)(*datap);
                    break;
                case 2:
                    stream << *(uint16_t*)datap;
                    break;
                case 4:
                    stream << *(uint32_t*)datap;
                    break;
                case 8:
                    stream << *(uint64_t*)datap;
                    break;
                default:
                    FUS_ASSERTD(0);
                    break;
                }
            }
            stream << std::endl;
            break;
        case net_field_t::data_type::e_string:
            stream << "        - DATA: '" << (char16_t*)datap << "'" << std::endl;
            break;
        }

        offset += msg->m_fields[i].m_datasz;
    }

    stream << "--- END   NETMSG ---" << std::endl;
}
