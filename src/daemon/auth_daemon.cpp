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

#include "auth.h"
#include "common.h"
#include "daemon_base.h"
#include "errors.h"

// =================================================================================

namespace fus
{
    struct auth_daemon_t
    {
        secure_daemon_t m_secure;
    };
};

static fus::auth_daemon_t* s_authDaemon = nullptr;

// =================================================================================

void fus::auth_daemon_init()
{
    FUS_ASSERTD(s_authDaemon == nullptr);

    s_authDaemon = (auth_daemon_t*)malloc(sizeof(auth_daemon_t));
    secure_daemon_init((secure_daemon_t*)s_authDaemon, ST_LITERAL("auth"));
}

bool fus::auth_daemon_running()
{
    return s_authDaemon != nullptr;
}

void fus::auth_daemon_close()
{
    FUS_ASSERTD(s_authDaemon);

    secure_daemon_close((secure_daemon_t*)s_authDaemon);
    free(s_authDaemon);
    s_authDaemon = nullptr;
}

// =================================================================================

static void auth_connection_encrypted(fus::auth_client_t* client, ssize_t result)
{
    /// fixme
    if (result < 0) {
        // ...
    }

    fus::auth_client_read(client);
}

static void auth_connect_buffer_read(fus::auth_client_t* client, ssize_t nread, void* buf)
{
    /// fixme
    if (nread < 0) {
        // ...
    }

    fus::secure_daemon_encrypt_stream((fus::secure_daemon_t*)s_authDaemon,
                                      (fus::crypt_stream_t*)client,
                                      (fus::crypt_established_cb)auth_connection_encrypted);
}

void fus::auth_daemon_accept_client(fus::auth_client_t* client)
{
    FUS_ASSERTD(s_authDaemon);

    // Unencrypted auth connection prefix:
    //     uint32_t msgsz
    //     uuid (junk)
    // Read in as a buffer to accept any arbitrary trash without choking too badly
    tcp_stream_read_msg((tcp_stream_t*)client, fus::protocol::connection_buffer::net_struct, (tcp_read_cb)auth_connect_buffer_read);
}
