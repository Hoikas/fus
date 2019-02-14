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
#include "daemon/daemon_base.h"
#include "daemon/server.h"
#include "core/errors.h"
#include <new>
#include <openssl/evp.h>
#include "protocol/common.h"

// =================================================================================

fus::admin_daemon_t* s_adminDaemon = nullptr;

// =================================================================================

void fus::admin_daemon_init()
{
    FUS_ASSERTD(s_adminDaemon == nullptr);

    s_adminDaemon = (admin_daemon_t*)malloc(sizeof(admin_daemon_t));
    db_trans_daemon_init((db_trans_daemon_t*)s_adminDaemon, ST_LITERAL("admin"));
    s_adminDaemon->m_hashCtx = EVP_MD_CTX_new();
    new(&s_adminDaemon->m_clients) FUS_LIST_DECL(admin_server_t, m_link);
}

bool fus::admin_daemon_running()
{
    return s_adminDaemon != nullptr;
}

void fus::admin_daemon_free()
{
    FUS_ASSERTD(s_adminDaemon);

    s_adminDaemon->m_clients.~list_declare();
    EVP_MD_CTX_free(s_adminDaemon->m_hashCtx);
    secure_daemon_free((secure_daemon_t*)s_adminDaemon);
    free(s_adminDaemon);
    s_adminDaemon = nullptr;
}

void fus::admin_daemon_shutdown()
{
    FUS_ASSERTD(s_adminDaemon);
    db_trans_daemon_shutdown((db_trans_daemon_t*)s_adminDaemon);

    // Clients will be removed from the list by admin_server_free
    auto it = s_adminDaemon->m_clients.front();
    while (it) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)it);
        it = s_adminDaemon->m_clients.next(it);
    }
}

// =================================================================================

static void admin_connection_encrypted(fus::admin_server_t* client, ssize_t result)
{
    if (result < 0 || (fus::admin_daemon_flags() & fus::daemon_t::e_shuttingDown)) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    s_adminDaemon->m_clients.push_back(client);
    fus::admin_server_read(client);
}

void fus::admin_daemon_accept(fus::admin_server_t* client, const void* msgbuf)
{
    // Validate connection
    if (!s_adminDaemon || (fus::admin_daemon_flags() & fus::daemon_t::e_shuttingDown) || !daemon_verify_connection((daemon_t*)s_adminDaemon, msgbuf, false)) {
        tcp_stream_shutdown((tcp_stream_t*)client);
        return;
    }

    // Init
    admin_server_init(client);
    fus::secure_daemon_encrypt_stream((fus::secure_daemon_t*)s_adminDaemon,
                                      (fus::crypt_stream_t*)client,
                                      (fus::crypt_established_cb)admin_connection_encrypted);
}
