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

#ifndef __FUS_SQLITE3DB_DAEMON_PRIVATE_H
#define __FUS_SQLITE3DB_DAEMON_PRIVATE_H

#include "daemon/daemon_base.h"
#include "io/hash.h"
#include <sqlite3.h>
#include "sqlite3db.h"

namespace fus
{
    class uuid;

    namespace sqlite3
    {
        struct db_daemon_t : public secure_daemon_t
        {
            FUS_LIST_DECL(db_server_t, m_link) m_clients;
            fus::hash m_hash;

            ::sqlite3* m_db;
            sqlite3_stmt* m_createAcctStmt;
            sqlite3_stmt* m_authAcctStmt;
        };

        extern db_daemon_t* s_dbDaemon;

        class query
        {
            sqlite3_stmt* m_stmt;

        public:
            query(sqlite3_stmt* stmt)
                : m_stmt(stmt)
            {
            }

            ~query()
            {
                sqlite3_clear_bindings(m_stmt);
                sqlite3_reset(m_stmt);
            }

            void bind(int idx, const ST::string& value);
            void bind(int idx, const std::string_view& value);
            void bind(int idx, const std::u16string_view& value);
            void bind(int idx, int value);
            void bind(int idx, const void* buf, size_t bufsz);
            void bind(int idx, const uuid& buf);

            template<typename T>
            T column(int idx) const;

            int step() { return sqlite3_step(m_stmt); }
        };
    };
};

// =================================================================================

template<typename _Msg>
using _db_cb = void(fus::sqlite3::db_server_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_db_cb<_Msg>>
static inline void db_read(fus::sqlite3::db_server_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>(client, (fus::tcp_read_cb)cb);
}

#endif
