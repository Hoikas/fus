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

#include "auth_private.h"
#include "core/errors.h"
#include "daemon/server.h"
#include <new>
#include "protocol/common.h"

// =================================================================================

fus::auth_daemon_t* s_authDaemon = nullptr;

// =================================================================================

static void auth_db_connected(fus::db_client_t* db, ssize_t status)
{
    ST::string addr = fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db);
    if (status == 0) {
        s_authDaemon->m_log.write_error("DB '{}' connection established!", addr);
        s_authDaemon->m_flags |= e_dbConnected;
    } else if (s_authDaemon->m_flags & e_shuttingDown) {
        s_authDaemon->m_log.write_info("DB '{}' connection abandoned", addr);
    } else {
        s_authDaemon->m_log.write_error("DB '{}' connection failed, retrying in 5s... Detail: {}",
                                        addr, uv_strerror(status));
        fus::client_reconnect((fus::client_t*)db, 5000);
    }
}

static void auth_db_disconnected(fus::db_client_t* db)
{
    // Caution: db could point to freed memory if we are shutting down...
    if (s_authDaemon->m_flags & e_shuttingDown) {
        s_authDaemon->m_log.write_info("DB '{}' connection shutdown");
        s_authDaemon->m_db = nullptr;
    } else {
        ST::string addr = fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db);
        s_authDaemon->m_log.write_error("DB '{}' connection lost, reconnecting in 5s...",
                                        fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db));
        fus::client_reconnect((fus::client_t*)db, 5000);
    }
    s_authDaemon->m_flags &= ~e_dbConnected;
}

// =================================================================================

void fus::auth_daemon_init()
{
    FUS_ASSERTD(s_authDaemon == nullptr);

    s_authDaemon = (auth_daemon_t*)malloc(sizeof(auth_daemon_t));
    secure_daemon_init((secure_daemon_t*)s_authDaemon, ST_LITERAL("auth"));
    s_authDaemon->m_log.set_level(server::get()->config().get<const ST::string&>("log", "level"));
    s_authDaemon->m_log.open(uv_default_loop(), ST_LITERAL("auth_daemon"));
    s_authDaemon->m_flags = 0;

    s_authDaemon->m_db = (db_client_t*)malloc(sizeof(db_client_t));
    FUS_ASSERTD(db_client_init(s_authDaemon->m_db, uv_default_loop()) == 0);
    tcp_stream_close_cb((tcp_stream_t*)s_authDaemon->m_db, (uv_close_cb)auth_db_disconnected);
    {
        sockaddr_storage addr;
        server::get()->config2addr(ST_LITERAL("db"), &addr);

        void* header = alloca(db_client_header_size());
        server::get()->fill_connection_header(header);

        unsigned int g = server::get()->config().get<unsigned int>("crypt", "db_g");
        const ST::string& n = server::get()->config().get<const ST::string&>("crypt", "db_n");
        const ST::string& x = server::get()->config().get<const ST::string&>("crypt", "db_x");
        db_client_connect(s_authDaemon->m_db, (sockaddr*)&addr, header, db_client_header_size(),
                          g, n, x, (client_connect_cb)auth_db_connected);
    }

    new(&s_authDaemon->m_clients) FUS_LIST_DECL(auth_server_t, m_link);
}

bool fus::auth_daemon_running()
{
    return s_authDaemon != nullptr;
}

void fus::auth_daemon_free()
{
    FUS_ASSERTD(s_authDaemon);

    s_authDaemon->m_clients.~list_declare();
    secure_daemon_free((secure_daemon_t*)s_authDaemon);
    free(s_authDaemon);
    s_authDaemon = nullptr;
}

void fus::auth_daemon_shutdown()
{
    FUS_ASSERTD(s_authDaemon);
    s_authDaemon->m_flags |= e_shuttingDown;

    // OK to free the database connection now.
    tcp_stream_free_on_close((tcp_stream_t*)s_authDaemon->m_db, true);
    tcp_stream_shutdown((tcp_stream_t*)s_authDaemon->m_db);

    // Clients will be removed from the list by auth_server_free
    auto it = s_authDaemon->m_clients.front();
    while (it) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)it);
        it = s_authDaemon->m_clients.next(it);
    }
}

// =================================================================================

static void auth_connection_encrypted(fus::auth_server_t* client, ssize_t result)
{
    if (result < 0 || (s_authDaemon->m_flags & e_shuttingDown)) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    s_authDaemon->m_clients.push_back(client);
    fus::auth_server_read(client);
}

void fus::auth_daemon_accept(fus::auth_server_t* client, const void* msgbuf)
{
    // Validate connection
    if (!s_authDaemon || (s_authDaemon->m_flags & e_shuttingDown) || !daemon_verify_connection((daemon_t*)s_authDaemon, msgbuf, false)) {
        tcp_stream_shutdown((tcp_stream_t*)client);
        return;
    }

    // Init
    auth_server_init(client);
    fus::secure_daemon_encrypt_stream((fus::secure_daemon_t*)s_authDaemon,
                                      (fus::crypt_stream_t*)client,
                                      (fus::crypt_established_cb)auth_connection_encrypted);
}
