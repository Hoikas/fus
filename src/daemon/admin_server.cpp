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
#include "daemon_base.h"
#include "protocol/admin.h"

// =================================================================================

void fus::admin_server_init(fus::admin_server_t* client)
{
    crypt_stream_init((crypt_stream_t*)client);
}

void fus::admin_server_free(fus::admin_server_t* client)
{
    crypt_stream_free((crypt_stream_t*)client);
}

void fus::admin_server_shutdown(fus::admin_server_t* client)
{
    tcp_stream_shutdown((tcp_stream_t*)client, (uv_close_cb)admin_server_free);
}

// =================================================================================

static inline bool admin_check_read(fus::admin_server_t* client, ssize_t nread)
{
    if (nread < 0) {
        s_adminDaemon->m_log.write_debug("[{}]: Read failed: {}",
                                        fus::tcp_stream_peeraddr((fus::tcp_stream_t*)client),
                                        uv_strerror(nread));
        fus::admin_server_shutdown(client);
        return false;
    }
    return true;
}

static void admin_pingpong(fus::admin_server_t* client, ssize_t nread, fus::protocol::admin_pingRequest* msg)
{
    if (!admin_check_read(client, nread))
        return;

    fus::protocol::msg_std_header header;
    header.set_type(fus::protocol::admin2client::e_pingReply);
    fus::tcp_stream_write((fus::tcp_stream_t*)client, &header, sizeof(header));

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write((fus::tcp_stream_t*)client, msg, nread);

    // Continue reading
    fus::admin_server_read(client);
}

// =================================================================================

static void admin_msg_pump(fus::admin_server_t* client, ssize_t nread, fus::protocol::msg_std_header* msg)
{
    if (!admin_check_read(client, nread))
        return;

    switch (msg->get_type()) {
    case fus::protocol::client2admin::e_pingRequest:
        admin_read<fus::protocol::admin_pingRequest>(client, admin_pingpong);
        break;
    default:
        s_adminDaemon->m_log.write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        fus::admin_server_shutdown(client);
    }
}

void fus::admin_server_read(fus::admin_server_t* client)
{
    admin_read<protocol::msg_std_header>(client, admin_msg_pump);
}
