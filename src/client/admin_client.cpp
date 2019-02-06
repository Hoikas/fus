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

#include "admin_client.h"
#include "core/errors.h"
#include "io/io.h"
#include "protocol/admin.h"

// =================================================================================

template<typename _Msg>
using _admin_cb = void(fus::admin_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_admin_cb<_Msg>>
static inline void admin_read(fus::admin_client_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>((fus::tcp_stream_t*)client, (fus::tcp_read_cb)cb);
}

// =================================================================================

int fus::admin_client_init(fus::admin_client_t* client, uv_loop_t* loop)
{
    int result = client_init((client_t*)client, loop);
    if (result < 0)
        return result;

    ((client_t*)client)->m_proc = (client_pump_proc)admin_client_read;

    return 0;
}

void fus::admin_client_free(fus::admin_client_t* client)
{
    client_free((client_t*)client);
}

void fus::admin_client_shutdown(fus::admin_client_t* client)
{
    client_kill_trans((client_t*)client, net_error::e_remoteShutdown, UV_ECANCELED);

    uv_close_cb cb = nullptr;
    if (((tcp_stream_t*)client)->m_flags & tcp_stream_t::e_freeOnClose)
        cb = (uv_close_cb)admin_client_free;
    tcp_stream_shutdown((tcp_stream_t*)client, cb);
}

// =================================================================================

void fus::admin_client_connect(fus::admin_client_t* client, const sockaddr* addr, void* buf, size_t bufsz, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::connection_header*)buf;
    header->set_connType(protocol::e_protocolCli2Admin);
    fus::client_crypt_connect((client_t*)client, addr, buf, bufsz, cb);
}

size_t fus::admin_client_header_size()
{
    return sizeof(protocol::connection_header);
}

// =================================================================================

void fus::admin_client_ping(fus::admin_client_t* client, uint32_t pingTimeMs, fus::client_trans_cb cb)
{
    protocol::admin_msg<protocol::admin_pingRequest> msg;
    msg.m_header.set_type(protocol::client2admin::e_pingRequest);
    msg.m_contents.set_transId(client_gen_trans((client_t*)client, cb));
    msg.m_contents.set_pingTime(pingTimeMs);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
}

// =================================================================================

static void admin_pingpong(fus::admin_client_t* client, ssize_t nread, fus::protocol::admin_pingReply* reply)
{
    if (nread < 0) {
        fus::admin_client_shutdown(client);
        return;
    }

    fus::trans_map_t& trans = ((fus::client_t*)client)->m_trans;
    auto it = trans.find(reply->get_transId());
    if (it != trans.end()) {
        it->second.m_cb((fus::client_t*)client, fus::net_error::e_success, nread, reply);
        trans.erase(it);
    }

    fus::admin_client_read(client);
}

static void admin_client_pump(fus::admin_client_t* client, ssize_t nread, fus::protocol::msg_std_header* header)
{
    if (nread < 0) {
        fus::admin_client_shutdown(client);
        return;
    }

    switch (header->get_type()) {
    case fus::protocol::admin2client::e_pingReply:
        admin_read<fus::protocol::admin_pingReply>(client, admin_pingpong);
        break;
    default:
        fus::admin_client_shutdown(client);
        break;
    }
}

void fus::admin_client_read(fus::admin_client_t* client)
{
    admin_read<protocol::msg_std_header>(client, admin_client_pump);
}