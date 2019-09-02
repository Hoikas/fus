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

FUS_NET_STRUCT_BEGIN(auth, pingRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(pingTime)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_BUFFER_TINY(payload)
FUS_NET_STRUCT_END(auth, pingRequest)

FUS_NET_STRUCT_BEGIN(auth, clientRegisterRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(buildId)
FUS_NET_STRUCT_END(auth, clientRegisterRequest)

FUS_NET_STRUCT_BEGIN(auth, acctLoginRequest)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(challenge)
    FUS_NET_FIELD_STRING_UTF16(name, 64)
    FUS_NET_FIELD_BLOB(hash, 20)
    FUS_NET_FIELD_STRING_UTF16(token, 64)
    FUS_NET_FIELD_STRING_UTF16(os, 8)
FUS_NET_STRUCT_END(auth, acctLoginRequest)

// =================================================================================

FUS_NET_STRUCT_BEGIN(auth, pingReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(pingTime)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_BUFFER_TINY(payload)
FUS_NET_STRUCT_END(auth, pingReply)

FUS_NET_STRUCT_BEGIN(auth, clientRegisterReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(loginSalt)
FUS_NET_STRUCT_END(auth, clientRegisterReply)

FUS_NET_STRUCT_BEGIN(auth, acctLoginReply)
    FUS_NET_FIELD_UINT16(type)
    FUS_NET_FIELD_UINT32(transId)
    FUS_NET_FIELD_UINT32(result)
    FUS_NET_FIELD_UUID(uuid)
    FUS_NET_FIELD_UINT32(flags)
    FUS_NET_FIELD_UINT32(billingType)
    FUS_NET_FIELD_BLOB(droidKey, sizeof(uint32_t) * 4)
FUS_NET_STRUCT_END(auth, acctLoginReply)
