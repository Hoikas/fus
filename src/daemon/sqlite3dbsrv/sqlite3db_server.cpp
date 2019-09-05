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

#include "sqlite3db_private.h"
#include "core/errors.h"
#include "daemon/daemon_base.h"
#include "io/net_error.h"
#include <new>
#include "protocol/db.h"

// =================================================================================

void fus::sqlite3::db_server_init(db_server_t* client)
{
    tcp_stream_free_cb(client, (tcp_free_cb)db_server_free);
    crypt_stream_init(client);
    crypt_stream_must_encrypt(client);
    new(&client->m_link) FUS_LIST_LINK(db_server_t);
}

void fus::sqlite3::db_server_free(db_server_t* client)
{
    client->m_link.~list_link();
}

// =================================================================================

static inline fus::sqlite3::db_daemon_t* db_daemon()
{
    return fus::sqlite3::s_dbDaemon;
}

static inline bool db_check_read(fus::sqlite3::db_server_t* client, ssize_t nread)
{
    if (nread < 0) {
        db_daemon()->m_log.write_debug("[{}]: Read failed: {}",
                                       fus::tcp_stream_peeraddr(client),
                                       uv_strerror(nread));
        fus::tcp_stream_shutdown(client);
        return false;
    }
    return true;
}

static void db_pingpong(fus::sqlite3::db_server_t* client, ssize_t nread, fus::protocol::db_pingRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write(client, msg, nread);

    // Continue reading
    fus::sqlite3::db_server_read(client);
}

// =================================================================================

static void db_acctCreate(fus::sqlite3::db_server_t* client, ssize_t nread, fus::protocol::db_acctCreateRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    fus::uuid uuid = fus::uuid::generate();
    fus::net_error result = fus::net_error::e_pending;
    {
        size_t hashBufsz = db_daemon()->m_hash.digestsz();
        void* hashBuf = alloca(hashBufsz);
        db_daemon()->m_hash.hash_account(ST::string::from_std_string(msg->get_name()),
                                         ST::string::from_std_string(msg->get_pass()),
                                         hashBuf, hashBufsz);

        fus::sqlite3::query query(db_daemon()->m_createAcctStmt);
        query.bind(1, msg->get_name());
        query.bind(2, hashBuf, hashBufsz);
        query.bind(3, uuid);
        query.bind(4, msg->get_flags());
        switch (query.step()) {
        case SQLITE_DONE:
            result = fus::net_error::e_success;
            break;
        case SQLITE_CONSTRAINT:
            // Account name is a case insensitive unique index :)
            result = fus::net_error::e_accountAlreadyExists;
            break;
        default:
            db_daemon()->m_log.write_error("SQLite3 Create Account Error: {}", sqlite3_errmsg(db_daemon()->m_db));
            result = fus::net_error::e_internalError;
            break;
        }
    }

    fus::protocol::db_acctCreateReply reply;
    reply.set_type(reply.id());
    reply.set_transId(msg->get_transId());
    reply.set_result((uint32_t)result);
    *reply.get_uuid() = uuid;
    fus::tcp_stream_write_msg(client, reply);

    // Continue reading
    fus::sqlite3::db_server_read(client);
}

// =================================================================================

static void db_acctAuth(fus::sqlite3::db_server_t* client, ssize_t nread, fus::protocol::db_acctAuthRequest* msg)
{
    if (!db_check_read(client, nread))
        return;

    fus::net_error result = fus::net_error::e_pending;
    fus::uuid acctUuid = fus::uuid::null;
    uint32_t acctFlags = 0;
    size_t hashbufsz = db_daemon()->m_hash.digestsz();
    if (hashbufsz != msg->get_hashsz()) {
        db_daemon()->m_log.write_error("ERROR: Account '{}' sent an unexpected digest length [sent: {}] [expected: {}]",
                                       msg->get_name(), msg->get_hashsz(), hashbufsz);
        result = fus::net_error::e_invalidParameter;
    } else {
        fus::sqlite3::query query(db_daemon()->m_authAcctStmt);
        query.bind(1, msg->get_name());
        switch (query.step()) {
        case SQLITE_DONE:
            result = fus::net_error::e_accountNotFound;
            break;

        case SQLITE_ROW:
        {
            auto dbAcctHash = query.column<std::tuple<const void*, size_t>>(0);
            void* hashbuf = alloca(hashbufsz);
            db_daemon()->m_hash.hash_login(std::get<0>(dbAcctHash), std::get<1>(dbAcctHash),
                                           msg->get_cliChallenge(), msg->get_srvChallenge(),
                                           hashbuf, hashbufsz);

            if (memcmp(hashbuf, std::get<0>(dbAcctHash), hashbufsz) == 0) {
                result = fus::net_error::e_success;
                acctUuid = query.column<fus::uuid>(1);
                acctFlags = query.column<int>(2);
            } else {
                result = fus::net_error::e_authenticationFailed;
            }
        }
        break;

        default:
            db_daemon()->m_log.write_error("SQLite3 Account Authenticate Error: {}", sqlite3_errmsg(db_daemon()->m_db));
            result = fus::net_error::e_internalError;
            break;
        }
    }

    fus::protocol::db_acctAuthReply reply;
    reply.set_type(reply.id());
    reply.set_transId(msg->get_transId());
    reply.set_result((uint32_t)result);
    reply.set_name(msg->get_name());
    *reply.get_uuid() = acctUuid;
    reply.set_flags(acctFlags);
    fus::tcp_stream_write_msg(client, reply);

    // Continue reading
    fus::sqlite3::db_server_read(client);
}

// =================================================================================

static void db_msg_pump(fus::sqlite3::db_server_t* client, ssize_t nread, fus::protocol::common_msg_std_header* msg)
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
        fus::sqlite3::s_dbDaemon->m_log.write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        fus::tcp_stream_shutdown(client);
    }
}

void fus::sqlite3::db_server_read(db_server_t* client)
{
    tcp_stream_peek_msg<protocol::common_msg_std_header>(client, (tcp_read_cb)db_msg_pump);
}
