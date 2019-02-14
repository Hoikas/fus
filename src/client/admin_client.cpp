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

int fus::admin_client_init(fus::admin_client_t* client, uv_loop_t* loop)
{
    int result = client_init((client_t*)client, loop);
    if (result < 0)
        return result;
    tcp_stream_free_cb((tcp_stream_t*)client, (tcp_free_cb)admin_client_free);

    ((client_t*)client)->m_proc = (client_pump_proc)admin_client_read;
    client->m_wallcb = nullptr;

    return 0;
}

void fus::admin_client_free(fus::admin_client_t* client)
{
    client_free((client_t*)client);
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

void fus::admin_client_connect(fus::admin_client_t* client, const sockaddr* addr, void* buf, size_t bufsz,
                               uint32_t g, const ST::string& n, const ST::string& x, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::connection_header*)buf;
    header->set_connType(protocol::e_protocolCli2Admin);
    fus::client_crypt_connect((client_t*)client, addr, buf, bufsz, g, n, x, cb);
}

size_t fus::admin_client_header_size()
{
    return sizeof(protocol::connection_header);
}

// =================================================================================

void fus::admin_client_wall_handler(fus::admin_client_t* client, fus::admin_client_wall_cb cb)
{
    client->m_wallcb = cb;
}

// =================================================================================

void fus::admin_client_ping(fus::admin_client_t* client, uint32_t pingTimeMs, fus::client_trans_cb cb,
                            void* instance, uint32_t transId)
{
    protocol::admin_msg<protocol::admin_pingRequest> msg;
    msg.m_header.set_type(protocol::client2admin::e_pingRequest);
    msg.m_contents.set_transId(client_gen_trans((client_t*)client, instance, transId, cb));
    msg.m_contents.set_pingTime(pingTimeMs);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
}

void fus::admin_client_wall(fus::admin_client_t* client, const ST::string& text)
{
    protocol::admin_msg<protocol::admin_wallRequest> msg;
    msg.m_header.set_type(protocol::client2admin::e_wallRequest);
    msg.m_contents.set_text(text);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
}

void fus::admin_client_create_account(fus::admin_client_t* client, const ST::string& name,
                                      const ST::string& pass, uint32_t flags, client_trans_cb cb,
                                      void* instance, uint32_t transId)
{
    protocol::admin_msg<protocol::admin_acctCreateRequest> msg;
    msg.m_header.set_type(protocol::client2admin::e_acctCreateRequest);
    msg.m_contents.set_transId(client_gen_trans((client_t*)client, instance, transId, cb));
    msg.m_contents.set_name(name);
    msg.m_contents.set_pass(pass);
    msg.m_contents.set_flags(flags);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
}

// =================================================================================

template<typename _Msg>
using _admin_cb = void(fus::admin_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb = _admin_cb<_Msg>>
static inline void admin_read(fus::admin_client_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>((fus::tcp_stream_t*)client, (fus::tcp_read_cb)cb);
}

template<typename _Msg>
static void admin_trans(fus::admin_client_t* client, ssize_t nread, _Msg* reply)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    fus::client_fire_trans((fus::client_t*)client, reply->get_transId(),
                           (fus::net_error)reply->get_result(), nread, reply);
    fus::admin_client_read(client);
}

template<typename _Msg>
static void admin_trans_noresult(fus::admin_client_t* client, ssize_t nread, _Msg* reply)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    fus::client_fire_trans((fus::client_t*)client, reply->get_transId(), fus::net_error::e_success, nread, reply);
    fus::admin_client_read(client);
}

// =================================================================================

static void admin_wallBCast(fus::admin_client_t* client, ssize_t nread, fus::protocol::admin_wallBCast* bcast)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    if (client->m_wallcb)
        client->m_wallcb(client, bcast->get_sender(), bcast->get_text());

    fus::admin_client_read(client);
}

static void admin_client_pump(fus::admin_client_t* client, ssize_t nread, fus::protocol::msg_std_header* header)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    switch (header->get_type()) {
    case fus::protocol::admin2client::e_pingReply:
        admin_read<fus::protocol::admin_pingReply>(client, admin_trans_noresult);
        break;
    case fus::protocol::admin2client::e_wallBCast:
        admin_read<fus::protocol::admin_wallBCast>(client, admin_wallBCast);
        break;
    case fus::protocol::admin2client::e_acctCreateReply:
        admin_read<fus::protocol::admin_acctCreateReply>(client, admin_trans);
        break;
    default:
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        break;
    }
}

void fus::admin_client_read(fus::admin_client_t* client)
{
    admin_read<protocol::msg_std_header>(client, admin_client_pump);
}
