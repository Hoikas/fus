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

#include "errors.h"
#include "uuid.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <rpc.h>
#else
#   include <uuid/uuid.h>
#endif

// =================================================================================

fus::uuid fus::uuid::null{};

fus::uuid fus::uuid::generate()
{
    uint8_t data[16];
#ifdef _WIN32
    UuidCreate((UUID*)data);
#else
    uuid_generate((uuid_t)data));
#endif
    return fus::uuid(data);
}

// =================================================================================

ST::string fus::uuid::as_string() const
{
#ifdef _WIN32
    RPC_CSTR str;
    FUS_ASSERTD(UuidToStringA((UUID*)m_data, &str) == RPC_S_OK);
    ST::string result = ST::string::from_utf8((char*)str);
    FUS_ASSERTD(RpcStringFreeA(&str) == RPC_S_OK);
    return result;
#else
    char str[37];
    uuid_unparse((uuid_t)m_data, str);
    return ST::string::from_utf8(str, 36);
#endif
}

bool fus::uuid::from_string(const char* str)
{
#if _WIN32
    return UuidFromStringA((RPC_CSTR)str, (UUID*)m_data) == RPC_S_OK;
#else
    return uuid_parse((char*)str, (uuid_t)m_data) == 0;
#endif
}

// =================================================================================

bool fus::uuid::empty() const
{
#if _WIN32
    RPC_STATUS status;
    int result = UuidIsNil((UUID*)m_data, &status);
    FUS_ASSERTD(result == RPC_S_OK);
    return result == 1;
#else
    return uuid_is_null((uuid_t)m_data) == 1;
#endif
}

bool fus::uuid::equals(const fus::uuid& rhs) const
{
#ifdef _WIN32
    RPC_STATUS status;
    signed int result = UuidCompare((UUID*)m_data, (UUID*)rhs.m_data, &status);
    FUS_ASSERTD(result == RPC_S_OK);
    return result == 0;
#else
    return uuid_compare((uuid_t)m_data, (uuid_t)rhs.m_data) == 0;
#endif
}

bool fus::uuid::operator <(const fus::uuid& rhs) const
{
#ifdef _WIN32
    RPC_STATUS status;
    signed int result = UuidCompare((UUID*)m_data, (UUID*)rhs.m_data, &status);
    FUS_ASSERTD(result == RPC_S_OK);
    return result == -1;
#else
    return uuid_compare((uuid_t)m_data, (uuid_t)rhs.m_data) == -1;
#endif
}
