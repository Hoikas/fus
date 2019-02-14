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

int fus::sqlite3_query::step()
{
    return sqlite3_step(m_stmt);
}
