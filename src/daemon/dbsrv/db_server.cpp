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
    tcp_stream_free_cb(client, (tcp_free_cb)db_server_free);
    crypt_stream_init(client);
    crypt_stream_must_encrypt(client);
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
        s_dbDaemon->m_log.write_debug("[{}]: Read failed: {}",
                                      fus::tcp_stream_peeraddr(client),
                                      uv_strerror(nread));
        fus::tcp_stream_shutdown(client);
        return false;
    }
    return true;
}

static void db_pingpong(fus::db_server_t* client, ssize_t nread, fus::protocol::db_pingRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write(client, msg, nread);

    // Continue reading
    fus::db_server_read(client);
}

// =================================================================================

static void db_acctCreated(fus::db_server_t* client, uint32_t transId, fus::net_error result, const fus::uuid& uuid)
{
    fus::protocol::db_acctCreateReply msg;
    msg.set_type(msg.id());
    msg.set_transId(transId);
    msg.set_result((uint32_t)result);
    *msg.get_uuid() = uuid;
    fus::tcp_stream_write_msg(client, msg);
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
    s_dbDaemon->m_db->create_account(msg->get_name(), msg->get_pass(), msg->get_flags(),
                                     (fus::database_acct_create_cb)db_acctCreated,
                                     client, msg->get_transId());

    // Continue reading
    fus::db_server_read(client);
}

// =================================================================================

static void db_acctAuthed(fus::db_server_t* client, uint32_t transId, fus::net_error result,
                          const std::u16string_view& name, const fus::uuid& uuid, uint32_t flags)
{
    fus::protocol::db_acctAuthReply msg;
    msg.set_type(msg.id());
    msg.set_transId(transId);
    msg.set_result((uint32_t)result);
    msg.set_name(name);

    // Ensure that no details leak to an externally facing process
    if (result == fus::net_error::e_success) {
        *msg.get_uuid() = uuid;
        msg.set_flags(flags);
    }

    fus::tcp_stream_write_msg(client, msg);
}

static void db_acctAuth(fus::db_server_t* client, ssize_t nread, fus::protocol::db_acctAuthRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    s_dbDaemon->m_db->authenticate_account(msg->get_name(), msg->get_cliChallenge(),
                                           msg->get_srvChallenge(), (fus::hash_type)msg->get_hashType(),
                                           msg->get_hash(), msg->get_hashsz(),
                                           (fus::database_acct_auth_cb)db_acctAuthed, client,
                                           msg->get_transId());

    // Continue reading
    fus::db_server_read(client);
}

// =================================================================================

static void db_msg_pump(fus::db_server_t* client, ssize_t nread, fus::protocol::common_msg_std_header* msg)
{
    if (!db_check_read(client, nread))
        return;

    switch (msg->get_type()) {
    case fus::protocol::db_pingRequest::id():
        db_read<fus::protocol::db_pingRequest>(client, db_pingpong);
        break;
    case fus::protocol::db_acctCreateRequest::id():
        db_read<fus::protocol::db_acctCreateRequest>(client, db_acctCreate);
        break;
    case fus::protocol::db_acctAuthRequest::id():
        db_read<fus::protocol::db_acctAuthRequest>(client, db_acctAuth);
        break;
    default:
        s_dbDaemon->m_log.write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        fus::tcp_stream_shutdown(client);
    }
}

void fus::db_server_read(fus::db_server_t* client)
{
    tcp_stream_peek_msg<protocol::common_msg_std_header>(client, (tcp_read_cb)db_msg_pump);
}
