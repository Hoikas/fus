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
#include "core/errors.h"
#include "daemon_base.h"
#include "protocol/auth.h"
#include "protocol/common.h"

// =================================================================================

#define FUS_ALLOCA_SEND(reply_struct, buf_name, header_name, reply_name) \
    size_t buf_name##sz = sizeof(reply_struct) + sizeof(fus::protocol::msg_std_header); \
    uint8_t* buf_name = (uint8_t*)alloca(buf_name##sz); \
    fus::protocol::msg_std_header* header_name = (fus::protocol::msg_std_header*)buf_name; \
    reply_struct* reply_name = (reply_struct*)buf_name + sizeof(fus::protocol::msg_std_header);

template<typename _Msg>
using _auth_cb = void(fus::auth_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_auth_cb<_Msg>>
static inline void auth_read(fus::auth_client_t* client, _Cb cb)
{
    fus::crypt_stream_read_msg<_Msg>((fus::crypt_stream_t*)client, (fus::tcp_read_cb)cb);
}

// =================================================================================

static void auth_pingpong(fus::auth_client_t* client, ssize_t nread, fus::protocol::auth_pingRequest* msg)
{
    FUS_ALLOCA_SEND(fus::protocol::auth_pingReply, buf, header, reply);
    header->set_type(fus::protocol::auth2client::e_pingReply);
    reply->set_transId(msg->get_transId());
    reply->set_pingTime(msg->get_pingTime());
    /// fixme: potential flooding/ddos, even with the BUFFER_TINY protection.
    if (msg->get_payloadsz())
        memcpy(reply->get_payload(), msg->get_payload(), msg->get_payloadsz());
    fus::crypt_stream_write((fus::crypt_stream_t*)client, buf, bufsz);
}

// =================================================================================

static void auth_msg_pump(fus::auth_client_t* client, ssize_t nread, fus::protocol::msg_std_header* msg)
{
    switch (msg->get_type()) {
    case fus::protocol::client2auth::e_pingRequest:
        auth_read<fus::protocol::auth_pingRequest>(client, auth_pingpong);
        break;
    default:
        /// fixme
        fus::crypt_stream_close((fus::crypt_stream_t*)client);
    }
}

void fus::auth_client_read(fus::auth_client_t* client)
{
    crypt_stream_read_msg<protocol::msg_std_header>((crypt_stream_t*)client, (tcp_read_cb)auth_msg_pump);
}
