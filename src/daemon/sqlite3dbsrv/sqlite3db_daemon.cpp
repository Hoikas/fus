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
#include "daemon/daemon_base.h"
#include "daemon/server.h"
#include <new>
#include "protocol/common.h"
#include "sqlite3db_private.h"

// =================================================================================

const char* s_schema =
    "BEGIN TRANSACTION; "

    // -- Table: Accounts
    "CREATE TABLE IF NOT EXISTS Accounts ("
    "idx INTEGER PRIMARY KEY AUTOINCREMENT, "
    "Name VARCHAR NOT NULL, Hash CHAR (64) NOT NULL, "
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

const ST::string s_addAcct =
    ST_LITERAL("INSERT INTO Accounts (Name, Hash, Uuid, Flags) "
               "VALUES (?, ?, ?, ?);");

const ST::string s_authAcct =
    ST_LITERAL("SELECT Hash, Uuid, Flags FROM Accounts "
               "WHERE Name = ? COLLATE NOCASE;");

// =================================================================================

fus::sqlite3::db_daemon_t* fus::sqlite3::s_dbDaemon = nullptr;

// =================================================================================

static inline bool init_stmt(sqlite3_stmt** stmt, const char* name, const ST::string& sql)
{
    using namespace fus::sqlite3;

    if (sqlite3_prepare_v2(s_dbDaemon->m_db, sql.c_str(), sql.size() + 1, stmt, nullptr) != SQLITE_OK) {
        s_dbDaemon->m_log.write_error("{} Failed: {}", name, sqlite3_errmsg(s_dbDaemon->m_db));
        return false;
    }
    return true;
}

// =================================================================================

bool fus::sqlite3::db_daemon_init()
{
    FUS_ASSERTD(s_dbDaemon == nullptr);

    s_dbDaemon = (db_daemon_t*)calloc(sizeof(db_daemon_t), 1);
    secure_daemon_init(s_dbDaemon, ST_LITERAL("db"));
    new(&s_dbDaemon->m_clients) FUS_LIST_DECL(db_server_t, m_link);
    new(&s_dbDaemon->m_hash) fus::hash(fus::hash_type::e_sha1);

    // If we're using a new database and its directory does not exist, bad things will happen.
    const ST::string& db_path = server::get()->config().get<const ST::string&>("sqlite", "path");
    std::filesystem::path db_dir = db_path.to_path().parent_path();
    std::error_code error;
    std::filesystem::create_directories(db_dir, error);
    if (error) {
        s_dbDaemon->m_log.write_error("Error creating directory '{}': {}", db_dir, error.message());
        // intentionally continuing to sqlite3_open
    }

    int result = sqlite3_open(db_path.c_str(), &s_dbDaemon->m_db);
    if (result != SQLITE_OK) {
        s_dbDaemon->m_log.write_error("SQLite3 Open Failed: {}", sqlite3_errstr(result));
        return false;
    }

    // Ensure all tables inited
    // Musing: perhaps we should have a table chose columns are (TableName, Version) for upgrading
    // purposes? As of right now, I don't envision this schema changing much once a feature is
    // implemented. URU is quite stale, after all...
    if (sqlite3_exec(s_dbDaemon->m_db, s_schema, nullptr, nullptr, nullptr) != SQLITE_OK) {
        s_dbDaemon->m_log.write_error("SQLite3 Schema Init Failed: {}", sqlite3_errmsg(s_dbDaemon->m_db));
        return false;
    }

    // Compile all SQL
    if (!init_stmt(&s_dbDaemon->m_createAcctStmt, "SQLite3 CreateAccountStmt Init", s_addAcct))
        return false;
    if (!init_stmt(&s_dbDaemon->m_authAcctStmt, "SQLite3 AuthAccountStmt Init", s_authAcct))
        return false;

    s_dbDaemon->m_log.write_info("SQLite3 Database Initialized: {}", db_path);
    return true;
}

bool fus::sqlite3::db_daemon_running()
{
    return s_dbDaemon != nullptr;
}

bool fus::sqlite3::db_daemon_shutting_down()
{
    if (s_dbDaemon)
        return s_dbDaemon->m_flags & daemon_t::e_shuttingDown;
    return false;
}


void fus::sqlite3::db_daemon_free()
{
    FUS_ASSERTD(s_dbDaemon);

    sqlite3_finalize(s_dbDaemon->m_createAcctStmt);
    sqlite3_finalize(s_dbDaemon->m_authAcctStmt);
    FUS_ASSERTD(sqlite3_close(s_dbDaemon->m_db) == SQLITE_OK);

    s_dbDaemon->m_hash.~hash();
    s_dbDaemon->m_clients.~list_declare();
    secure_daemon_free(s_dbDaemon);
    free(s_dbDaemon);
    s_dbDaemon = nullptr;
}

void fus::sqlite3::db_daemon_shutdown()
{
    FUS_ASSERTD(s_dbDaemon);
    secure_daemon_shutdown(s_dbDaemon);

    // Clients will be removed from the list by db_server_free
    auto it = s_dbDaemon->m_clients.front();
    while (it) {
        tcp_stream_shutdown(it);
        it = s_dbDaemon->m_clients.next(it);
    }
}

// =================================================================================

static void db_connection_encrypted(fus::sqlite3::db_server_t* client, ssize_t result)
{
    if (result < 0 || (fus::sqlite3::s_dbDaemon->m_flags & fus::daemon_t::e_shuttingDown)) {
        fus::tcp_stream_shutdown(client);
        return;
    }

    fus::sqlite3::s_dbDaemon->m_clients.push_back(client);
    fus::sqlite3::db_server_read(client);
}

void fus::sqlite3::db_daemon_accept(db_server_t* client, const void* msgbuf)
{
    // Validate connection
    if (!s_dbDaemon || (s_dbDaemon->m_flags & daemon_t::e_shuttingDown) || !daemon_verify_connection(s_dbDaemon, msgbuf, false)) {
        tcp_stream_shutdown(client);
        return;
    }

    // Init
    db_server_init(client);
    fus::secure_daemon_encrypt_stream(s_dbDaemon, client, (crypt_established_cb)db_connection_encrypted);
}
