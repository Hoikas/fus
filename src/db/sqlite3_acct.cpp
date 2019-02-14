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
#include "io/log_file.h"

// =================================================================================

void fus::db_sqlite3::authenticate_account(const ST::string& name, const void* hashBuf,
                                           size_t hashBufsz, const void* seedBuf, size_t seedBufsz,
                                           database_acct_auth_cb cb, void* instance, uint32_t transId)
{
    if (cb)
        cb(instance, transId, net_error::e_notSupported, nullptr, 0, uuid::null, 0);
}

void fus::db_sqlite3::create_account(const ST::string& name, const void* hashBuf, size_t hashBufsz,
                                     uint32_t flags, database_acct_create_cb cb, void* instance, uint32_t transId)
{
    uuid uuid = uuid::generate();
    net_error result = net_error::e_pending;
    {
        sqlite3_query query(m_createAcctStmt);
        query.bind(1, name);
        query.bind(2, hashBuf, hashBufsz);
        query.bind(3, uuid);
        query.bind(4, flags);
        switch(query.step()) {
        case SQLITE_DONE:
            result = net_error::e_success;
            break;
        case SQLITE_CONSTRAINT:
            // Account name is a case insensitive unique index :)
            result = net_error::e_accountAlreadyExists;
            break;
        default:
            m_log->write_error("SQLite3 Create Account Error: {}", sqlite3_errmsg(m_db));
            result = net_error::e_internalError;
            break;
        }
    }

    if (cb)
        cb(instance, transId, result, uuid);
}
