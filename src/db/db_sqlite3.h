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

#ifndef __FUS_DB_SQLITE3_H
#define __FUS_DB_SQLITE3_H

#include "database.h"
#include <sqlite3.h>

namespace fus
{
    class uuid;

    class db_sqlite3 final : public database
    {
        sqlite3* m_db;
        sqlite3_stmt* m_createAcctStmt;
        sqlite3_stmt* m_authAcctStmt;

        static const char* s_schema;
        static const ST::string s_addAcct;
        static const ST::string s_authAcct;

    protected:
        friend class database;

        db_sqlite3()
            : m_db(), m_createAcctStmt(), m_authAcctStmt()
        { }

        bool open(const fus::config_parser&) override;
        bool init_stmt(sqlite3_stmt**, const char*, const ST::string&);

    public:
        ~db_sqlite3();

    public:
        void authenticate_account(const ST::string& name, const void* hashBuf,
                                  size_t hashBufsz, const void* saltBuf, size_t saltBufsz,
                                  database_acct_auth_cb cb, void* instance, uint32_t transId) override;
        void create_account(const ST::string& name, const void* hashBuf, size_t hashBufsz,
                            uint32_t flags, database_acct_create_cb, void* instance, uint32_t transId) override;
    };

    class sqlite3_query
    {
        sqlite3_stmt* m_stmt;

    public:
        sqlite3_query(sqlite3_stmt* stmt)
            : m_stmt(stmt)
        { }
        ~sqlite3_query();

        void bind(int idx, const ST::string& value);
        void bind(int idx, int value);
        void bind(int idx, const void* buf, size_t bufsz);
        void bind(int idx, const uuid& buf);

        int step();
    };
};

#endif
