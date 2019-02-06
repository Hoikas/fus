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

#include "client_base.h"
#include "core/errors.h"
#include <new>
#include <openssl/bn.h>

// =================================================================================

int fus::client_init(fus::client_t* client, uv_loop_t* loop)
{
    int result = tcp_stream_init((tcp_stream_t*)client, loop);
    if (result < 0)
        return result;
    crypt_stream_init((crypt_stream_t*)client);

    // Init members
    client->m_connectcb = nullptr;
    client->m_transId = 0;
    new(&client->m_trans) std::map<uint32_t, transaction_t>;

    return 0;
}

void fus::client_free(fus::client_t* client)
{
    client_kill_trans(client, net_error::e_remoteShutdown, UV_ECANCELED);
    client->m_trans.~map();
    crypt_stream_free((crypt_stream_t*)client);
}

// =================================================================================

struct connect_req_t
{
    enum
    {
        e_encrypt = (1<<0),
    };

    uv_connect_t m_req;
    uint32_t m_flags;
    size_t m_bufsz;
    uint8_t m_buf[];
};

static void _client_encrypted(fus::client_t* client, ssize_t status)
{
    if (client->m_connectcb)
        client->m_connectcb(client, status);
    if (status >= 0)
        client->m_proc(client);
}

static void _client_connected(connect_req_t* req, int status)
{
    fus::client_t* client = ((fus::client_t*)req->m_req.handle);
    if (status < 0) {
        if (client->m_connectcb)
            client->m_connectcb(client, status);
    } else {
        ((fus::tcp_stream_t*)client)->m_flags |= fus::tcp_stream_t::e_connected;
        fus::tcp_stream_write((fus::tcp_stream_t*)client, req->m_buf, req->m_bufsz);
        if (req->m_flags & connect_req_t::e_encrypt) {
            fus::crypt_stream_establish_client((fus::crypt_stream_t*)client, (fus::crypt_established_cb)_client_encrypted);
        } else {
            if (client->m_connectcb)
                client->m_connectcb(client, 0);
            client->m_proc(client);
        }
    }

    free(req);
}

// =================================================================================

void fus::client_crypt_connect(fus::client_t* client, const sockaddr* addr, const void* buf, size_t bufsz, client_connect_cb cb)
{
    FUS_ASSERTD(client);
    client->m_connectcb = cb;

    connect_req_t* req = (connect_req_t*)malloc(sizeof(connect_req_t) + bufsz);
    req->m_flags = connect_req_t::e_encrypt;
    req->m_bufsz = bufsz;
    memcpy(req->m_buf, buf, bufsz);

    uv_tcp_connect((uv_connect_t*)req, (uv_tcp_t*)client, addr, (uv_connect_cb)_client_connected);
}

void fus::client_connect(fus::client_t* client, const sockaddr* addr, const void* buf, size_t bufsz, client_connect_cb cb)
{
    FUS_ASSERTD(client);
    client->m_connectcb = cb;

    connect_req_t* req = (connect_req_t*)malloc(sizeof(connect_req_t) + bufsz);
    req->m_flags = 0;
    req->m_bufsz = bufsz;
    memcpy(req->m_buf, buf, bufsz);

    uv_tcp_connect((uv_connect_t*)req, (uv_tcp_t*)client, addr, (uv_connect_cb)_client_connected);
}

// =================================================================================

uint32_t fus::client_next_transId(fus::client_t* client)
{
    return client->m_transId++;
}

uint32_t fus::client_gen_trans(fus::client_t* client, fus::client_trans_cb cb)
{
    uint32_t transId = client->m_transId++;
    if (cb)
        client->m_trans[transId] = { transId, cb };
    return transId;
}

void fus::client_kill_trans(fus::client_t* client, net_error result, ssize_t nread)
{
    for (auto& it : client->m_trans)
        it.second.m_cb(client, result, nread, nullptr);
    client->m_trans.clear();
}
