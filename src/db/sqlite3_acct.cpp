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
#include <tuple>

// =================================================================================

void fus::db_sqlite3::authenticate_account(const std::u16string_view& name, uint32_t cliChallenge,
                                           uint32_t srvChallenge, hash_type hashType,
                                           const void* hashBuf, size_t hashBufsz,
                                           database_acct_auth_cb cb, void* instance,
                                           uint32_t transId)
{
    net_error result = net_error::e_pending;
    uuid acctUuid = uuid::null;
    uint32_t acctFlags = 0;
    {
        sqlite3_query query(m_authAcctStmt);
        query.bind(1, name);
        switch (query.step()) {
        case SQLITE_DONE:
            result = net_error::e_accountNotFound;
            break;
        case SQLITE_ROW:
            {
                acctFlags = query.column<int>(2);
                auto acctHashType = hash_type::e_unspecified;
                extract_hash(acctFlags, acctHashType);
                if (acctHashType != hashType) {
                    m_log->write_error("Hash type mismatch when authenticating account '{}'", name);
                    result = net_error::e_invalidParameter;
                    break;
                }

                auto acctHash = query.column<std::tuple<const void*, size_t>>(0);
                if (compare_hash(cliChallenge, srvChallenge, hashType, std::get<0>(acctHash),
                                 std::get<1>(acctHash), hashBuf, hashBufsz)) {
                    result = net_error::e_success;
                    acctUuid = query.column<fus::uuid>(1);
                } else {
                    result = net_error::e_authenticationFailed;
                }
            }
            break;
        default:
            m_log->write_error("SQLite3 Authenticate Account Error: {}", sqlite3_errmsg(m_db));
            result = net_error::e_internalError;
            break;
        }
    }

    if (cb)
        cb(instance, transId, result, name, acctUuid, acctFlags);
}

void fus::db_sqlite3::create_account(const std::u16string_view& name, const std::string_view& pass,
                                     uint32_t flags, database_acct_create_cb cb, void* instance,
                                     uint32_t transId)
{
    uuid uuid = uuid::generate();
    net_error result = net_error::e_pending;
    {
        hash_type hashType = hash_type::e_sha1;
        extract_hash(flags, hashType);

        size_t hashBufsz = digestsz(hashType);
        void* hashBuf = alloca(hashBufsz);
        hash_account(name, pass, hashType, hashBuf, hashBufsz);

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
