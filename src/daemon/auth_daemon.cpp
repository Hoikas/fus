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
#include <new>
#include "protocol/common.h"
#include "server.h"

// =================================================================================

fus::auth_daemon_t* s_authDaemon = nullptr;

// =================================================================================

void fus::auth_daemon_init()
{
    FUS_ASSERTD(s_authDaemon == nullptr);

    s_authDaemon = (auth_daemon_t*)malloc(sizeof(auth_daemon_t));
    secure_daemon_init((secure_daemon_t*)s_authDaemon, ST_LITERAL("auth"));
    s_authDaemon->m_log.set_level(server::get()->config().get<const ST::string&>("log", "level"));
    s_authDaemon->m_log.open(uv_default_loop(), ST_LITERAL("auth_daemon"));
    s_authDaemon->m_flags = 0;
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
