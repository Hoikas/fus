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

#include "core/errors.h"
#include "core/uuid.h"
#include "db_sqlite3.h"
#include <tuple>

// =================================================================================

fus::sqlite3_query::~sqlite3_query()
{
    sqlite3_clear_bindings(m_stmt);
    sqlite3_reset(m_stmt);
}

// =================================================================================

void fus::sqlite3_query::bind(int idx, const ST::string& value)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_text(m_stmt, idx, value.c_str(), value.size(), nullptr) == SQLITE_OK);
}

void fus::sqlite3_query::bind(int idx, const std::string_view& value)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_text(m_stmt, idx, value.data(), value.size(), nullptr) == SQLITE_OK);
}

void fus::sqlite3_query::bind(int idx, const std::u16string_view& value)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_text16(m_stmt, idx, value.data(), value.size() * sizeof(char16_t),
                                    nullptr) == SQLITE_OK);
}

void fus::sqlite3_query::bind(int idx, int value)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_int(m_stmt, idx, value) == SQLITE_OK);
}

void fus::sqlite3_query::bind(int idx, const void* buf, size_t bufsz)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_blob(m_stmt, idx, buf, bufsz, nullptr) == SQLITE_OK);
}

void fus::sqlite3_query::bind(int idx, const fus::uuid& uuid)
{
    FUS_ASSERTD(idx > 0);
    FUS_ASSERTD(sqlite3_bind_blob(m_stmt, idx, uuid.data(), sizeof(fus::uuid), nullptr) == SQLITE_OK);
}

// =================================================================================

template<>
ST::string fus::sqlite3_query::column<ST::string>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));

    ST::char_buffer buf;
    buf.allocate(sqlite3_column_bytes(m_stmt, idx));
    memcpy(buf.data(), sqlite3_column_text(m_stmt, idx), buf.size());
    return ST::string(buf);
}

template<>
std::string_view fus::sqlite3_query::column<std::string_view>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));
    return std::string_view((const char*)sqlite3_column_text(m_stmt, idx),
                            sqlite3_column_bytes(m_stmt, idx));
}

template<>
std::u16string_view fus::sqlite3_query::column<std::u16string_view>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));
    return std::u16string_view((const char16_t*)sqlite3_column_text16(m_stmt, idx),
                               sqlite3_column_bytes16(m_stmt, idx) / sizeof(char16_t));
}

template<>
int fus::sqlite3_query::column<int>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));
    return sqlite3_column_int(m_stmt, idx);
}

template<>
std::tuple<const void*, size_t> fus::sqlite3_query::column<std::tuple<const void*, size_t>>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));
    return std::make_tuple(sqlite3_column_blob(m_stmt, idx), sqlite3_column_bytes(m_stmt, idx));
}

template<>
fus::uuid fus::sqlite3_query::column<fus::uuid>(int idx)
{
    FUS_ASSERTD(idx < sqlite3_data_count(m_stmt));
    FUS_ASSERTD(sqlite3_column_bytes(m_stmt, idx) == sizeof(fus::uuid));
    return fus::uuid(sqlite3_column_blob(m_stmt, idx));
}

// =================================================================================

int fus::sqlite3_query::step()
{
    return sqlite3_step(m_stmt);
}
