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
#include "db/constants.h"
#include "db_client.h"
#include "protocol/db.h"

// =================================================================================

int fus::db_client_init(fus::db_client_t* client, uv_loop_t* loop)
{
    int result = client_init(client, loop);
    if (result < 0)
        return result;
    tcp_stream_free_cb(client, (tcp_free_cb)db_client_free);

    client->m_proc = (client_pump_proc)db_client_read;

    return 0;
}

void fus::db_client_free(fus::db_client_t* client)
{
    client_free(client);
}

// =================================================================================

void fus::db_client_connect(fus::db_client_t* client, const sockaddr* addr, void* buf, size_t bufsz, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::common_connection_header*)buf;
    header->set_connType(protocol::e_protocolSrv2Database);
    fus::client_crypt_connect(client, addr, buf, bufsz, cb);
}

void fus::db_client_connect(fus::db_client_t* client, const sockaddr* addr, void* buf, size_t bufsz,
    uint32_t g, const ST::string& n, const ST::string& x, fus::client_connect_cb cb)
{
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    auto header = (protocol::common_connection_header*)buf;
    header->set_connType(protocol::e_protocolSrv2Database);
    fus::client_crypt_connect(client, addr, buf, bufsz, g, n, x, cb);
}

size_t fus::db_client_header_size()
{
    return sizeof(protocol::common_connection_header);
}

// =================================================================================

template<typename _Msg>
using _db_cb = void(fus::db_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb = _db_cb<_Msg>>
static inline void db_read(fus::db_client_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>(client, (fus::tcp_read_cb)cb);
}

template<typename _Msg>
static void db_trans(fus::db_client_t* client, ssize_t nread, _Msg* reply)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown(client);
        return;
    }

    fus::client_fire_trans(client, reply->get_transId(), (fus::net_error)reply->get_result(), nread, reply);
    fus::db_client_read(client);
}

template<typename _Msg>
static void db_trans_noresult(fus::db_client_t* client, ssize_t nread, _Msg* reply)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown(client);
        return;
    }

    fus::client_fire_trans(client, reply->get_transId(), fus::net_error::e_success, nread, reply);
    fus::db_client_read(client);
}

// =================================================================================

static void db_client_pump(fus::db_client_t* client, ssize_t nread, fus::protocol::common_msg_std_header* header)
{
    if (nread < 0) {
        fus::tcp_stream_shutdown(client);
        return;
    }

    switch (header->get_type()) {
    case fus::protocol::db_pingReply::id():
        db_read<fus::protocol::db_pingReply>(client, db_trans_noresult);
        break;
    case fus::protocol::db_acctCreateReply::id():
        db_read<fus::protocol::db_acctCreateReply>(client, db_trans);
        break;
    case fus::protocol::db_acctAuthReply::id():
        db_read<fus::protocol::db_acctAuthReply>(client, db_trans);
        break;
    default:
        fus::tcp_stream_shutdown(client);
        break;
    }
}

void fus::db_client_read(fus::db_client_t* client)
{
    tcp_stream_peek_msg<protocol::common_msg_std_header>(client, (tcp_read_cb)db_client_pump);
}
