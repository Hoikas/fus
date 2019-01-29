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
#include "daemon_base.h"
#include <openssl/rand.h>
#include "protocol/auth.h"

// =================================================================================

void fus::auth_client_init(fus::auth_client_t* client)
{
    crypt_stream_init((crypt_stream_t*)client);
    client->m_flags = 0;
    client->m_loginSalt = 0;
}

// =================================================================================

static void auth_pingpong(fus::auth_client_t* client, ssize_t nread, fus::protocol::auth_pingRequest* msg)
{
    fus::protocol::msg_std_header header;
    header.set_type(fus::protocol::auth2client::e_pingReply);
    fus::crypt_stream_write((fus::crypt_stream_t*)client, &header, sizeof(header));

    // Message reply is a bitwise copy, so we'll just throw the request back.
    fus::crypt_stream_write((fus::crypt_stream_t*)client, msg, nread);
}

static void auth_registerClient(fus::auth_client_t* client, ssize_t nread, fus::protocol::auth_clientRegisterRequest* msg)
{
    // If a client tries to register more than once, they are obviously being nefarious.
    if (client->m_flags & e_clientRegistered)
        fus::crypt_stream_close((fus::crypt_stream_t*)client);

    RAND_bytes((unsigned char*)(&client->m_loginSalt), sizeof(client->m_loginSalt));
    client->m_flags |= e_clientRegistered;

    fus::protocol::auth_msg<fus::protocol::auth_clientRegisterReply> reply;
    reply.m_header.set_type(fus::protocol::auth2client::e_clientRegisterReply);
    reply.m_contents.set_loginSalt(client->m_loginSalt);
    fus::crypt_stream_write((fus::crypt_stream_t*)client, reply, sizeof(reply));
}

// =================================================================================

static void auth_msg_pump(fus::auth_client_t* client, ssize_t nread, fus::protocol::msg_std_header* msg)
{
    switch (msg->get_type()) {
    case fus::protocol::client2auth::e_pingRequest:
        auth_read<fus::protocol::auth_pingRequest>(client, auth_pingpong);
        break;
    case fus::protocol::client2auth::e_clientRegisterRequest:
        auth_read<fus::protocol::auth_clientRegisterRequest>(client, auth_registerClient);
        break;
    default:
        s_authDaemon->m_log.write_error("Received unimplemented message type 0x{04X} -- kicking client", msg->get_type());
        /// fixme
        fus::crypt_stream_close((fus::crypt_stream_t*)client);
    }
}

void fus::auth_client_read(fus::auth_client_t* client)
{
    auth_read<protocol::msg_std_header>(client, auth_msg_pump);
}
