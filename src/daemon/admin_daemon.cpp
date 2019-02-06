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

#include "admin_private.h"
#include "core/errors.h"
#include <new>
#include "protocol/common.h"
#include "server.h"

// =================================================================================

fus::admin_daemon_t* s_adminDaemon = nullptr;

// =================================================================================

void fus::admin_daemon_init()
{
    FUS_ASSERTD(s_adminDaemon == nullptr);

    s_adminDaemon = (admin_daemon_t*)malloc(sizeof(admin_daemon_t));
    secure_daemon_init((secure_daemon_t*)s_adminDaemon, ST_LITERAL("admin"));
    s_adminDaemon->m_log.set_level(server::get()->config().get<const ST::string&>("log", "level"));
    s_adminDaemon->m_log.open(uv_default_loop(), ST_LITERAL("admin_daemon"));
    new(&s_adminDaemon->m_clients) FUS_LIST_DECL(admin_server_t, m_link);
}

bool fus::admin_daemon_running()
{
    return s_adminDaemon != nullptr;
}

void fus::admin_daemon_free()
{
    FUS_ASSERTD(s_adminDaemon);

    secure_daemon_free((secure_daemon_t*)s_adminDaemon);
    free(s_adminDaemon);
    s_adminDaemon = nullptr;
}

// =================================================================================

static void admin_connection_encrypted(fus::admin_server_t* client, ssize_t result)
{
    if (result < 0) {
        fus::admin_server_shutdown(client);
        return;
    }

    s_adminDaemon->m_clients.push_back(client);
    fus::admin_server_read(client);
}

void fus::admin_daemon_accept(fus::admin_server_t* client, const void* msgbuf)
{
    // Validate connection
    if (!s_adminDaemon || !daemon_verify_connection((daemon_t*)s_adminDaemon, msgbuf, false)) {
        tcp_stream_shutdown((tcp_stream_t*)client);
        return;
    }

    // Init
    admin_server_init(client);
    fus::secure_daemon_encrypt_stream((fus::secure_daemon_t*)s_adminDaemon,
                                      (fus::crypt_stream_t*)client,
                                      (fus::crypt_established_cb)admin_connection_encrypted);
}
