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

#include "db/database.h"
#include "db_private.h"
#include "daemon/daemon_base.h"
#include "daemon/server.h"
#include "core/errors.h"
#include <new>
#include "protocol/common.h"

// =================================================================================

fus::db_daemon_t* s_dbDaemon = nullptr;

// =================================================================================

bool fus::db_daemon_init()
{
    FUS_ASSERTD(s_dbDaemon == nullptr);

    s_dbDaemon = (db_daemon_t*)malloc(sizeof(db_daemon_t));
    secure_daemon_init(s_dbDaemon, ST_LITERAL("db"));
    s_dbDaemon->m_db = database::init(server::get()->config(), s_dbDaemon->m_log);
    new(&s_dbDaemon->m_clients) FUS_LIST_DECL(db_server_t, m_link);

    if (!s_dbDaemon->m_db) {
        db_daemon_free();
        return false;
    }
    return true;
}

bool fus::db_daemon_running()
{
    return s_dbDaemon != nullptr;
}

bool fus::db_daemon_shutting_down()
{
    if (s_dbDaemon)
        return s_dbDaemon->m_flags & daemon_t::e_shuttingDown;
    return false;
}


void fus::db_daemon_free()
{
    FUS_ASSERTD(s_dbDaemon);

    s_dbDaemon->m_clients.~list_declare();
    delete s_dbDaemon->m_db;
    secure_daemon_free(s_dbDaemon);
    free(s_dbDaemon);
    s_dbDaemon = nullptr;
}

void fus::db_daemon_shutdown()
{
    FUS_ASSERTD(s_dbDaemon);
    secure_daemon_shutdown(s_dbDaemon);

    // Clients will be removed from the list by db_server_free
    auto it = s_dbDaemon->m_clients.front();
    while (it) {
        fus::tcp_stream_shutdown(it);
        it = s_dbDaemon->m_clients.next(it);
    }
}

// =================================================================================

static void db_connection_encrypted(fus::db_server_t* client, ssize_t result)
{
    if (result < 0 || (s_dbDaemon->m_flags & fus::daemon_t::e_shuttingDown)) {
        fus::tcp_stream_shutdown(client);
        return;
    }

    s_dbDaemon->m_clients.push_back(client);
    fus::db_server_read(client);
}

void fus::db_daemon_accept(fus::db_server_t* client, const void* msgbuf)
{
    // Validate connection
    if (!s_dbDaemon || (s_dbDaemon->m_flags & fus::daemon_t::e_shuttingDown) || !daemon_verify_connection(s_dbDaemon, msgbuf, false)) {
        tcp_stream_shutdown(client);
        return;
    }

    // Init
    db_server_init(client);
    fus::secure_daemon_encrypt_stream(s_dbDaemon, client, (crypt_established_cb)db_connection_encrypted);
}
