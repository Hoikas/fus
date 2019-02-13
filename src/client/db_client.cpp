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

#include "core/errors.h"
#include <cstring>
#include "db_client.h"
#include "protocol/db.h"

// =================================================================================

int fus::db_client_init(fus::db_client_t* client, uv_loop_t* loop)
{
    int result = client_init((client_t*)client, loop);
    if (result < 0)
        return result;
    tcp_stream_free_cb((tcp_stream_t*)client, (tcp_free_cb)db_client_free);

    ((client_t*)client)->m_proc = (client_pump_proc)db_client_read;

    return 0;
}

void fus::db_client_free(fus::db_client_t* client)
{
    client_free((client_t*)client);
}

// =================================================================================

void fus::db_client_connect(fus::db_client_t* client, const sockaddr* addr, void* buf, size_t bufsz, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::connection_header*)buf;
    header->set_connType(protocol::e_protocolSrv2Database);
    fus::client_crypt_connect((client_t*)client, addr, buf, bufsz, cb);
}

void fus::db_client_connect(fus::db_client_t* client, const sockaddr* addr, void* buf, size_t bufsz,
    uint32_t g, const ST::string& n, const ST::string& x, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::connection_header*)buf;
    header->set_connType(protocol::e_protocolSrv2Database);
    fus::client_crypt_connect((client_t*)client, addr, buf, bufsz, g, n, x, cb);
}

size_t fus::db_client_header_size()
{
    return sizeof(protocol::connection_header);
}

// =================================================================================

void fus::db_client_ping(fus::db_client_t* client, uint32_t pingTimeMs, fus::client_trans_cb cb, void* param)
{
    protocol::db_msg<protocol::db_pingRequest> msg;
    msg.m_header.set_type(protocol::client2db::e_pingRequest);
    msg.m_contents.set_transId(client_gen_trans((client_t*)client, cb, param));
    msg.m_contents.set_pingTime(pingTimeMs);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
}

void fus::db_client_create_account(fus::db_client_t* client, const ST::string& name, const void* hashBuf,
                                   size_t hashBufsz, uint32_t flags, fus::client_trans_cb cb, void* param)
{
    protocol::db_msg<protocol::db_acctCreateRequest> msg;
    msg.m_header.set_type(protocol::client2db::e_acctCreateRequest);
    msg.m_contents.set_transId(client_gen_trans((client_t*)client, cb, param));
    msg.m_contents.set_name(name);
    msg.m_contents.set_flags(flags);
    msg.m_contents.set_hashsz(hashBufsz);
    tcp_stream_write((tcp_stream_t*)client, &msg, sizeof(msg));
    tcp_stream_write((tcp_stream_t*)client, hashBuf, hashBufsz);
}

// =================================================================================

template<typename _Msg>
using _db_cb = void(fus::db_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb = _db_cb<_Msg>>
static inline void db_read(fus::db_client_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>((fus::tcp_stream_t*)client, (fus::tcp_read_cb)cb);
}

template<typename _Msg>
static void db_trans(fus::db_client_t* client, ssize_t nread, _Msg* reply)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    fus::client_fire_trans((fus::client_t*)client, reply->get_transId(), nread, reply);
    fus::db_client_read(client);
}

// =================================================================================

static void db_client_pump(fus::db_client_t* client, ssize_t nread, fus::protocol::msg_std_header* header)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        return;
    }

    switch (header->get_type()) {
    case fus::protocol::db2client::e_pingReply:
        db_read<fus::protocol::db_pingReply>(client, db_trans);
        break;
    case fus::protocol::db2client::e_acctCreateReply:
        db_read<fus::protocol::db_acctCreateReply>(client, db_trans);
        break;
    default:
        fus::tcp_stream_shutdown((fus::tcp_stream_t*)client);
        break;
    }
}

void fus::db_client_read(fus::db_client_t* client)
{
    db_read<protocol::msg_std_header>(client, db_client_pump);
}
