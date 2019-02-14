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

#include "core/config_parser.h"
#include "core/errors.h"
#include "db_sqlite3.h"
#include <filesystem>
#include "io/log_file.h"

// =================================================================================

const char* fus::db_sqlite3::s_schema =
    "BEGIN TRANSACTION; "

    // -- Table: Accounts
    "CREATE TABLE IF NOT EXISTS Accounts ("
    "idx INTEGER PRIMARY KEY AUTOINCREMENT, "
    "Name VARCHAR NOT NULL, Hash CHAR (128) NOT NULL, "
    "Uuid CHAR (16) NOT NULL, Flags INTEGER NOT NULL); "

    "CREATE UNIQUE INDEX IF NOT EXISTS Index_AccountName ON Accounts (Name COLLATE NOCASE); "

    // -- Table: VaultNodes
    "CREATE TABLE IF NOT EXISTS VaultNodes ("
    "idx INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
    "CreateTime INTEGER, ModifyTime INTEGER, CreateAgeName VARCHAR (64), "
    "CreateAgeUuid CHAR (16), CreatorUuid CHAR (16), CreatorIdx INTEGER, "
    "NodeType INTEGER, Int32_1 INTEGER, Int32_2 INTEGER, Int32_3 INTEGER, "
    "Int32_4 INTEGER, UInt32_1 INTEGER, UInt32_2 INTEGER, UInt32_3 INTEGER, "
    "UInt32_4 INTEGER, Uuid_1 CHAR (16), Uuid_2 CHAR (16), Uuid_3 CHAR (16), "
    "Uuid_4 CHAR (16), String64_1 VARCHAR (64), String64_2 VARCHAR (64), "
    "String64_3 VARCHAR (64), String64_4 VARCHAR (64), String64_5 VARCHAR (64), "
    "String64_6 VARCHAR (64), IString64_1 VARCHAR (64), IString64_2 VARCHAR (64), "
    "Text_1 TEXT, Text_2 TEXT, Blob_1 BLOB, Blob_2 BLOB); "

    "COMMIT TRANSACTION;";

const ST::string fus::db_sqlite3::s_addAcct =
    ST_LITERAL("INSERT INTO Accounts (Name, Hash, Uuid, Flags) "
               "VALUES (?, ?, ?, ?);");

const ST::string fus::db_sqlite3::s_authAcct =
    ST_LITERAL("SELECT Hash, Uuid, Flags FROM Accounts "
               "WHERE Name = ? COLLATE NOCASE;");

// =================================================================================

bool fus::db_sqlite3::open(const fus::config_parser& config)
{
    // If we're using a new database and its directory does not exist, bad things will happen.
    const ST::string& db_path = config.get<const ST::string&>("sqlite", "path");
    std::filesystem::path db_dir = db_path.to_path().parent_path();
    std::error_code error;
    std::filesystem::create_directories(db_dir, error);

    int result = sqlite3_open(db_path.c_str(), &m_db);
    if (result != SQLITE_OK) {
        m_log->write_error("SQLite3 Open Failed: {}", sqlite3_errstr(result));
        return false;
    }

    // Ensure all tables inited
    // Musing: perhaps we should have a table chose columns are (TableName, Version) for upgrading
    // purposes? As of right now, I don't envision this schema changing much once a feature is
    // implemented. URU is quite stale, after all...
    if (sqlite3_exec(m_db, s_schema, nullptr, nullptr, nullptr) != SQLITE_OK) {
        m_log->write_error("SQLite3 Schema Init Failed: {}", sqlite3_errmsg(m_db));
        return false;
    }

    // Compile all SQL
    if (!init_stmt(&m_createAcctStmt, "SQLite3 CreateAccountStmt Init", s_addAcct))
        return false;
    if (!init_stmt(&m_authAcctStmt, "SQLite3 AuthAccountStmt Init", s_authAcct))
        return false;

    m_log->write_info("SQLite3 Database Initialized: {}", db_path);
    return true;
}

bool fus::db_sqlite3::init_stmt(sqlite3_stmt** stmt, const char* name, const ST::string& sql)
{
    if (sqlite3_prepare_v2(m_db, sql.c_str(), sql.size() + 1, stmt, nullptr) != SQLITE_OK) {
        m_log->write_error("{} Failed: {}", name, sqlite3_errmsg(m_db));
        return false;
    }
    return true;
}

fus::db_sqlite3::~db_sqlite3()
{
    sqlite3_finalize(m_createAcctStmt);
    sqlite3_finalize(m_authAcctStmt);
    if (m_db)
        FUS_ASSERTD(sqlite3_close(m_db) == SQLITE_OK);
}
