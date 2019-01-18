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

FUS_NET_STRUCT_BEGIN(connection_header)
    FUS_NET_FIELD_UINT8(connType)
    FUS_NET_FIELD_UINT16(msgsz)
    FUS_NET_FIELD_UINT32(buildId)
    FUS_NET_FIELD_UINT32(buildType)
    FUS_NET_FIELD_UINT32(branchId)
    FUS_NET_FIELD_UUID(product)
FUS_NET_STRUCT_END(connection_header)

FUS_NET_STRUCT_BEGIN(msg_std_header)
    FUS_NET_FIELD_UINT16(type)
FUS_NET_STRUCT_END(msg_std_header)

FUS_NET_STRUCT_BEGIN(msg_size_header)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT16(size)
FUS_NET_STRUCT_END(msg_size_header)
