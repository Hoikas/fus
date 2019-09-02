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
#include "client/db_client.h"
#include "core/errors.h"
#include "daemon/daemon_base.h"
#include "db/constants.h"
#include <new>
#include <openssl/rand.h>
#include "protocol/auth.h"
#include "protocol/db.h"
#include <regex>

// =================================================================================

void fus::auth_server_init(fus::auth_server_t* client)
{
    tcp_stream_free_cb(client, (tcp_free_cb)auth_server_free);
    crypt_stream_init(client);
    new(&client->m_link) FUS_LIST_LINK(auth_server_t);
    client->m_flags = 0;
    client->m_srvChallenge = 0;
}

void fus::auth_server_free(fus::auth_server_t* client)
{
    if (s_authDaemon->m_db)
        client_kill_trans(s_authDaemon->m_db, client, net_error::e_disconnected, UV_ECONNRESET, true);
    client->m_link.~list_link();
}

// =================================================================================

static inline bool auth_check_read(fus::auth_server_t* client, ssize_t nread)
{
    if (nread < 0) {
        s_authDaemon->m_log.write_debug("[{}] Read failed: {}",
                                        fus::tcp_stream_peeraddr(client),
                                        uv_strerror(nread));
        fus::tcp_stream_shutdown(client);
        return false;
    }
    return true;
}

static void auth_pingpong(fus::auth_server_t* client, ssize_t nread, fus::protocol::auth_pingRequest* msg)
{
    if (!auth_check_read(client, nread))
        return;

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::tcp_stream_write(client, msg, nread);

    // Continue reading
    fus::auth_server_read(client);
}

// =================================================================================

static void auth_registerClient(fus::auth_server_t* client, ssize_t nread, fus::protocol::auth_clientRegisterRequest* msg)
{
    if (!auth_check_read(client, nread))
        return;

    // If a client tries to register more than once, they are obviously being nefarious.
    if (client->m_flags & e_clientRegistered)
        fus::tcp_stream_shutdown(client);

    RAND_bytes((unsigned char*)(&client->m_srvChallenge), sizeof(client->m_srvChallenge));
    client->m_flags |= e_clientRegistered;

    fus::protocol::auth_clientRegisterReply reply;
    reply.set_type(reply.id());
    reply.set_loginSalt(client->m_srvChallenge);
    fus::tcp_stream_write_msg(client, reply);

    // Continue reading
    fus::auth_server_read(client);
}

// =================================================================================

static void auth_acctLoginAuthed(fus::auth_server_t* client, fus::db_client_t* db, uint32_t transId,
                                 fus::net_error result, ssize_t nread, const fus::protocol::db_acctAuthReply* reply)
{
    if (result == fus::net_error::e_success)
        s_authDaemon->m_log.write_debug("Account '{}' login: {}", reply->get_name(),
                                        fus::net_error_string(result));
    else
        s_authDaemon->m_log.write_error("Account '{}' login: {}", reply->get_name(),
                                           fus::net_error_string(result));

    // TODO: this needs to request the account's players from the database
    fus::protocol::auth_acctLoginReply msg;
    msg.set_type(msg.id());
    msg.set_transId(transId);
    msg.set_result((uint32_t)result);
    if (result == fus::net_error::e_success) {
        *msg.get_uuid() = reply->get_uuid();
        msg.set_flags(reply->get_flags());
        // TOOD: billingtype, droidkey
    }
    fus::tcp_stream_write_msg(client, msg);
}

static void auth_acctLogin(fus::auth_server_t* client, ssize_t nread, fus::protocol::auth_acctLoginRequest* msg)
{
    if (!auth_check_read(client, nread))
        return;

    fus::net_error result = fus::net_error::e_pending;
    do {
        if (!(client->m_flags & e_clientRegistered)) {
            s_authDaemon->m_log.write_debug("[{}] Wants to login but has not registered",
                                            fus::tcp_stream_peeraddr(client));
            result = fus::net_error::e_disconnected;
            break;
        }

        if (client->m_flags & e_acctLoggedIn || client->m_flags & e_acctLoginInProgress) {
            s_authDaemon->m_log.write_debug("[{}] Sent dupe login requests",
                                            fus::tcp_stream_peeraddr(client));
            result = fus::net_error::e_disconnected;
            break;
        }

        if (!(s_authDaemon->m_flags & fus::daemon_t::e_dbConnected)) {
            s_authDaemon->m_log.write_error("[{}] Tried to login to account '{}', but the DBSrv is unavailable",
                                            fus::tcp_stream_peeraddr(client),
                                            msg->get_name());
            result = fus::net_error::e_internalError;
            break;
        }

        // Is this one of those silly, broken sha0 account names (email address style)???
        static const std::regex re_domain("[^@]+@([^.]+\\.)*([^.]+)\\.[^.]+");
        std::cmatch match;
        std::regex_search(ST::string(msg->get_name()).to_lower().c_str(), match, re_domain);
        if (!(match.empty() || match[2].compare("gametap") == 0)) {
            s_authDaemon->m_log.write_debug("[{}] Sent an account name [{}] that would require SHA-0",
                                            fus::tcp_stream_peeraddr(client),
                                            msg->get_name());
            result = fus::net_error::e_notSupported;
            break;
        }
    } while(0);


    if (result != fus::net_error::e_pending) {
        fus::protocol::auth_acctLoginReply reply;
        reply.set_type(reply.id());
        reply.set_transId(msg->get_transId());
        reply.set_result((uint32_t)result);
        fus::tcp_stream_write_msg(client, reply);
        if (result == fus::net_error::e_disconnected)
            fus::tcp_stream_shutdown(client);
    } else {
        // eap-tastic networking code sends big endian 32-bit integers
        void* hash = alloca(msg->get_hashsz());
        {
            uint32_t* destp = (uint32_t*)hash;
            uint32_t* srcp = (uint32_t*)msg->get_hash();
            for (size_t i = 0; i < msg->get_hashsz() / sizeof(uint32_t); ++i)
                destp[i] = FUS_BE32(srcp[i]);
        }

        fus::protocol::db_acctAuthRequest fwd;
        fwd.set_type(fwd.id());
        fus::client_prep_trans(s_authDaemon->m_db, fwd, client, msg->get_transId(),
                               (fus::client_trans_cb)auth_acctLoginAuthed);
        fwd.set_name(msg->get_name());
        fwd.set_cliChallenge(msg->get_challenge());
        fwd.set_srvChallenge(client->m_srvChallenge);
        fwd.set_hashType((uint8_t)fus::hash_type::e_sha1);
        fwd.set_hashsz(msg->get_hashsz());
        tcp_stream_write_msg(s_authDaemon->m_db, fwd, hash, msg->get_hashsz());
    }

    // Continue reading
    fus::auth_server_read(client);
}

// =================================================================================

static void auth_msg_pump(fus::auth_server_t* client, ssize_t nread, fus::protocol::common_msg_std_header* msg)
{
    if (!auth_check_read(client, nread))
        return;

    switch (msg->get_type()) {
    case fus::protocol::auth_pingRequest::id():
        auth_read<fus::protocol::auth_pingRequest>(client, auth_pingpong);
        break;
    case fus::protocol::auth_clientRegisterRequest::id():
        auth_read<fus::protocol::auth_clientRegisterRequest>(client, auth_registerClient);
        break;
    case fus::protocol::auth_acctLoginRequest::id():
        auth_read<fus::protocol::auth_acctLoginRequest>(client, auth_acctLogin);
        break;
    default:
        s_authDaemon->m_log.write_error("[{}] Received unimplemented message type 0x{04X} -- kicking client",
                                        fus::tcp_stream_peeraddr(client), msg->get_type());
        fus::tcp_stream_shutdown(client);
        break;
    }
}

void fus::auth_server_read(fus::auth_server_t* client)
{
    tcp_stream_peek_msg<protocol::common_msg_std_header>(client, (tcp_read_cb)auth_msg_pump);
}
