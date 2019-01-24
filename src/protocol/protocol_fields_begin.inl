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

namespace fus
{
    namespace protocol
    {
        namespace _fields {
            template <typename _T, size_t _Sz>
            constexpr size_t size(_T(&)[_Sz]) { return _Sz; }
        }
    }
};

#define FUS_NET_STRUCT_BEGIN(name) \
    namespace fus { namespace protocol { namespace _fields { \
        static const fus::net_field_t name[] = {

#define FUS_NET_FIELD_BUFFER(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer, #name, 0 },

#define FUS_NET_FIELD_BUFFER_TINY(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer_tiny, #name, 0 },

#define FUS_NET_FIELD_BUFFER_HUGE(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer_huge, #name, 0 },

#define FUS_NET_FIELD_BUFFER_REDUNDANT(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer_redundant, #name, 0 },

#define FUS_NET_FIELD_BUFFER_REDUNDANT_TINY(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer_redundant_tiny, #name, 0 },

#define FUS_NET_FIELD_BUFFER_REDUNDANT_HUGE(name) \
    { fus::net_field_t::data_type::e_integer, #name##"sz", sizeof(uint32_t) }, \
    { fus::net_field_t::data_type::e_buffer_redundant_huge, #name, 0 },

#define FUS_NET_FIELD_UINT8(name) \
    { fus::net_field_t::data_type::e_integer, #name, sizeof(uint8_t) },

#define FUS_NET_FIELD_UINT16(name) \
    { fus::net_field_t::data_type::e_integer, #name, sizeof(uint16_t) },

#define FUS_NET_FIELD_UINT32(name) \
    { fus::net_field_t::data_type::e_integer, #name, sizeof(uint32_t) },

#define FUS_NET_FIELD_STRING(name, size) \
    { fus::net_field_t::data_type::e_string, #name, size * sizeof(char16_t) },

#define FUS_NET_FIELD_UUID(name) \
    { fus::net_field_t::data_type::e_blob, #name, 16 },

#define FUS_NET_STRUCT_END(name) \
    }; \
    }; }; }; \
    \
    namespace fus { namespace protocol { namespace _net_structs { \
        const fus::net_struct_t name =\
            { #name, fus::protocol::_fields::size(fus::protocol::_fields::name), \
              fus::protocol::_fields::name }; \
    }; }; };
