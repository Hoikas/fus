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

#define FUS_NET_STRUCT_BEGIN(name) \
    const fus::net_struct_t* fus::protocol::name::net_struct = &fus::protocol::_net_structs::name;

// noops
#define FUS_NET_FIELD_BLOB(name, size) ;
#define FUS_NET_FIELD_BUFFER(name) ;
#define FUS_NET_FIELD_BUFFER_TINY(name) ;
#define FUS_NET_FIELD_BUFFER_HUGE(name) ;
#define FUS_NET_FIELD_BUFFER_REDUNDANT(name) ;
#define FUS_NET_FIELD_BUFFER_REDUNDANT_TINY(name) ;
#define FUS_NET_FIELD_BUFFER_REDUNDANT_HUGE(name) ;
#define FUS_NET_FIELD_UINT8(name) ;
#define FUS_NET_FIELD_UINT16(name) ;
#define FUS_NET_FIELD_UINT32(name) ;
#define FUS_NET_FIELD_STRING_UTF8(name, size) ;
#define FUS_NET_FIELD_STRING_UTF16(name, size) ;
#define FUS_NET_FIELD_UUID(name) ;
#define FUS_NET_STRUCT_END(name) ;
