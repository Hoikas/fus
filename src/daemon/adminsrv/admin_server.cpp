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
#include "client/db_client.h"
#include "core/errors.h"
#include "daemon/daemon_base.h"
#include "db/constants.h"
#include <new>
#include <openssl/evp.h>
#include "protocol/admin.h"
#include "protocol/db.h"

// =================================================================================

void fus::admin_server_init(fus::admin_server_t* client)
{
    tcp_stream_free_cb(client, (tcp_free_cb)admin_server_free);
    crypt_stream_init(client);
    crypt_stream_must_encrypt(client);
    new(&client->m_link) FUS_LIST_LINK(admin_server_t);
}

void fus::admin_server_free(fus::admin_server_t* client)
{
    if (s_adminDaemon->m_db)
        client_kill_trans(s_adminDaemon->m_db, client, net_error::e_disconnected, UV_ECONNRESET, true);
    client->m_link.~list_link();
}

// =================================================================================

static inline bool admin_check_read(fus::admin_server_t* client, ssize_t nread)
{
    if (nread < 0) {
        s_adminDaemon->m_log.write_debug("[{}]: Read failed: {}",
                                         fus::tcp_stream_peeraddr(client),
                                         uv_strerror(nread));
        fus::tcp_stream_shutdown(client);
        return false;
    }
    return true;
}

static void admin_pingpong(fus::admin_server_t* client, ssize_t nread, fus::protocol::admin_pingRequest* msg)
{
    if (!admin_check_read(client, nread))
        return;

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write(client, msg, nread);

    // Continue reading
    fus::admin_server_read(client);
}

static void admin_wall(fus::admin_server_t* client, ssize_t nread, fus::protocol::admin_wallRequest* msg)
{
    if (!admin_check_read(client, nread))
        return;

    ST::string senderaddr = fus::tcp_stream_peeraddr(client);
    s_adminDaemon->m_log.write("[{}] wall: {}", senderaddr, msg->get_text_view());

    fus::protocol::admin_wallBCast bcast;
    bcast.set_type(fus::protocol::admin2client::e_wallBCast);
    bcast.set_sender(senderaddr);
    bcast.set_text(msg->get_text_view());

    auto it = s_adminDaemon->m_clients.front();
    while (it) {
        fus::tcp_stream_write_msg(it, bcast);
        it = s_adminDaemon->m_clients.next(it);
    }

    // Continue reading
    fus::admin_server_read(client);
}

static void admin_acctCreated(fus::admin_server_t* client, fus::db_client_t* db, uint32_t transId,
                              fus::net_error result, ssize_t nread, const fus::protocol::db_acctCreateReply* reply)
{
    fus::protocol::admin_acctCreateReply msg;
    msg.set_type(fus::protocol::admin2client::e_acctCreateReply);
    msg.set_transId(transId);
    msg.set_result((uint32_t)result);
    if (reply)
        *msg.get_uuid() = reply->get_uuid();
    fus::tcp_stream_write_msg(client, msg);
}

static void admin_acctCreate(fus::admin_server_t* client, ssize_t nread, fus::protocol::admin_acctCreateRequest* msg)
{
    if (!admin_check_read(client, nread))
        return;

    // Database client must be available for this to work. Otherwise, just phail.
    if (s_adminDaemon->m_flags & fus::daemon_t::e_dbConnected) {
        fus::protocol::db_acctCreateRequest fwd;
        fwd.set_type(fus::protocol::client2db::e_acctCreateRequest);
        fus::client_prep_trans(s_adminDaemon->m_db, fwd, client, msg->get_transId(),
                               (fus::client_trans_cb)admin_acctCreated);
        fwd.set_name(msg->get_name_view());
        fwd.set_pass(msg->get_pass_view());
        fwd.set_flags(msg->get_flags());
        fus::tcp_stream_write_msg(s_adminDaemon->m_db, fwd);
    } else {
        s_adminDaemon->m_log.write_error("[{}] Tried to create an account '{}', but the DBSrv is unavailable",
                                         fus::tcp_stream_peeraddr(client), msg->get_name());
        fus::protocol::admin_acctCreateReply reply;
        reply.set_type(fus::protocol::admin2client::e_acctCreateReply);
        reply.set_transId(msg->get_transId());
        reply.set_result((uint32_t)fus::net_error::e_internalError);
        fus::tcp_stream_write_msg(client, reply);
    }

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
    case fus::protocol::client2admin::e_wallRequest:
        admin_read<fus::protocol::admin_wallRequest>(client, admin_wall);
        break;
    case fus::protocol::client2admin::e_acctCreateRequest:
        admin_read<fus::protocol::admin_acctCreateRequest>(client, admin_acctCreate);
        break;
    default:
        s_adminDaemon->m_log.write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        fus::tcp_stream_shutdown(client);
        break;
    }
}

void fus::admin_server_read(fus::admin_server_t* client)
{
    tcp_stream_peek_msg<protocol::msg_std_header>(client, (tcp_read_cb)admin_msg_pump);
}
