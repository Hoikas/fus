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
#include "core/errors.h"
#include "daemon/daemon_base.h"
#include "io/net_error.h"
#include <new>
#include "protocol/db.h"

// =================================================================================

void fus::db_server_init(fus::db_server_t* client)
{
    tcp_stream_free_cb((tcp_stream_t*)client, (tcp_free_cb)db_server_free);
    crypt_stream_init((crypt_stream_t*)client);
    crypt_stream_must_encrypt((crypt_stream_t*)client);
    new(&client->m_link) FUS_LIST_LINK(db_server_t);
}

void fus::db_server_free(fus::db_server_t* client)
{
    client->m_link.~list_link();
}

// =================================================================================

static inline bool db_check_read(fus::db_server_t* client, ssize_t nread)
{
    if (nread < 0) {
        fus::db_daemon_log().write_debug("[{}]: Read failed: {}",
                                         fus::tcp_stream_peeraddr((fus::tcp_stream_t*)client),
                                         uv_strerror(nread));
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return false;
    }
    return true;
}

static void db_pingpong(fus::db_server_t* client, ssize_t nread, fus::protocol::db_pingRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    fus::protocol::msg_std_header header;
    header.set_type(fus::protocol::db2client::e_pingReply);
    fus::tcp_stream_write((fus::tcp_stream_t*)client, &header, sizeof(header));

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write((fus::tcp_stream_t*)client, msg, nread);

    // Continue reading
    fus::db_server_read(client);
}

// =================================================================================

static void db_acctCreated(fus::db_server_t* client, uint32_t transId, fus::net_error result, const fus::uuid& uuid)
{
    fus::protocol::db_msg<fus::protocol::db_acctCreateReply> msg;
    msg.m_header.set_type(fus::protocol::db2client::e_acctCreateReply);
    msg.m_contents.set_transId(transId);
    msg.m_contents.set_result((uint32_t)result);
    *msg.m_contents.get_uuid() = uuid;
    fus::tcp_stream_write_msg((fus::tcp_stream_t*)client, msg);
}

static void db_acctCreate(fus::db_server_t* client, ssize_t nread, fus::protocol::db_acctCreateRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    // Since this is the first database request implementing, I'm dumping my thoughts here.
    // It's my baby, so if you don't like it, write your own damn URU server and kiss my ass.
    // Anyway, there's going to be lots of seemingly pointless boilerplate code whose sole responsibility
    // is to copy data from the stream's read buffer into the proper database call. At the time of
    // this writing, we know this database is SQLite3. That's grand, except with SQLite3 we're not
    // trying to deal with the database blocking execution of the event loop. Gulp. In the future,
    // we will add another database backend, chosen at runtime. Therefore, we will need this copy-
    // pasta code to handle that runtime dispatching. Goal: sqlite database for quick local servers
    // and a real RDBMS backend for serious shards. Supplemental apps like a theoretical dirtsand
    // database converter can use the same API we do for migrations...
    // So, yeah, TL;DR: https://www.youtube.com/watch?v=NUexv1tuWZA
    s_dbDaemon->m_db->create_account(msg->get_name(), msg->get_hash(), msg->get_hashsz(), msg->get_flags(),
                                     (fus::database_acct_create_cb)db_acctCreated, client, msg->get_transId());

    // Continue reading
    fus::db_server_read(client);
}

// =================================================================================

static void db_msg_pump(fus::db_server_t* client, ssize_t nread, fus::protocol::msg_std_header* msg)
{
    if (!db_check_read(client, nread))
        return;

    switch (msg->get_type()) {
    case fus::protocol::client2db::e_pingRequest:
        db_read<fus::protocol::db_pingRequest>(client, db_pingpong);
        break;
    case fus::protocol::client2db::e_acctCreateRequest:
        db_read<fus::protocol::db_acctCreateRequest>(client, db_acctCreate);
        break;
    default:
        fus::db_daemon_log().write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
    }
}

void fus::db_server_read(fus::db_server_t* client)
{
    db_read<protocol::msg_std_header>(client, db_msg_pump);
}
