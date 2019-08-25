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

#ifndef __FUS_DB_DAEMON_PRIVATE_H
#define __FUS_DB_DAEMON_PRIVATE_H

#include "db.h"
#include "daemon/daemon_base.h"

namespace fus
{
    class database;

    struct db_daemon_t : public secure_daemon_t
    {
        database* m_db;
        FUS_LIST_DECL(db_server_t, m_link) m_clients;
    };
};

extern fus::db_daemon_t* s_dbDaemon;

// =================================================================================

template<typename _Msg>
using _db_cb = void(fus::db_server_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_db_cb<_Msg>>
static inline void db_read(fus::db_server_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>(client, (fus::tcp_read_cb)cb);
}

#endif
