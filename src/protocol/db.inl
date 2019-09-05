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

FUS_NET_STRUCT_BEGIN(db, pingRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(pingTime)
FUS_NET_STRUCT_END(db, pingRequest)

FUS_NET_STRUCT_BEGIN(db, acctCreateRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_STRING_UTF8(name, 64)
    FUS_NET_FIELD_STRING_UTF8(pass, 64)
    FUS_NET_FIELD_UINT32(flags)
FUS_NET_STRUCT_END(db, acctCreateRequest)

FUS_NET_STRUCT_BEGIN(db, acctAuthRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_STRING_UTF8(name, 64)
    FUS_NET_FIELD_UINT32(cliChallenge)
    FUS_NET_FIELD_UINT32(srvChallenge)
    FUS_NET_FIELD_BUFFER_TINY(hash)
FUS_NET_STRUCT_END(db, acctAuthRequest)

// =================================================================================

FUS_NET_STRUCT_BEGIN(db, pingReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(pingTime)
FUS_NET_STRUCT_END(db, pingReply)

FUS_NET_STRUCT_BEGIN(db, acctCreateReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(result)
    FUS_NET_FIELD_UUID(uuid)
FUS_NET_STRUCT_END(db, acctCreateReply)

FUS_NET_STRUCT_BEGIN(db, acctAuthReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(result)
    FUS_NET_FIELD_STRING_UTF8(name, 64)
    FUS_NET_FIELD_UUID(uuid)
    FUS_NET_FIELD_UINT32(flags)
FUS_NET_STRUCT_END(db, acctAuthReply)
