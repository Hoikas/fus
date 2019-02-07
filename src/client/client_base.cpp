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
#include <string_theory/st_codecs.h>

// =================================================================================

struct connect_req_t
{
    enum
    {
        e_encrypt = (1<<0),
        e_ownsKeys = (1<<1),
    };

    uv_connect_t m_req;
    sockaddr_storage m_sockaddr;
    uint32_t m_flags;
    uint32_t m_g;
    BIGNUM* m_nKey;
    BIGNUM* m_xKey;
    size_t m_bufsz;
    uint8_t m_buf[];
};

// =================================================================================

int fus::client_init(fus::client_t* client, uv_loop_t* loop)
{
    int result = tcp_stream_init((tcp_stream_t*)client, loop);
    if (result < 0)
        return result;
    crypt_stream_init((crypt_stream_t*)client);

    // Init members
    result = uv_timer_init(loop, &client->m_reconnect);
    tcp_stream_ref((tcp_stream_t*)client, (uv_handle_t*)&client->m_reconnect);

    client->m_connectReq = nullptr;
    client->m_connectcb = nullptr;
    client->m_transId = 0;
    new(&client->m_trans) std::map<uint32_t, transaction_t>;

    return result;
}

void fus::client_free(fus::client_t* client)
{
    if (client->m_connectReq && client->m_connectReq->m_flags & connect_req_t::e_ownsKeys) {
        BN_free(client->m_connectReq->m_nKey);
        BN_free(client->m_connectReq->m_xKey);
    }
    uv_timer_stop(&client->m_reconnect);
    uv_close((uv_handle_t*)&client->m_reconnect, tcp_stream_unref);
    client_kill_trans(client, net_error::e_remoteShutdown, UV_ECANCELED);
    client->m_trans.~map();
}

// =================================================================================

static inline void _copy_sockaddr(connect_req_t* req, const sockaddr* addr)
{
    // We do this because, in the case of a reconnect, uv_tcp_getpeeraddr returns an error because 
    // we are dealing with a reinitialized uv_tcp_t under the hood.
    if (addr->sa_family == AF_INET)
        memcpy(&req->m_sockaddr, addr, sizeof(sockaddr_in));
    else if (addr->sa_family == AF_INET6)
        memcpy(&req->m_sockaddr, addr, sizeof(sockaddr_in6));
    else
        FUS_ASSERTR(0);
}

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
            if (req->m_flags & connect_req_t::e_ownsKeys)
                fus::crypt_stream_set_keys_client((fus::crypt_stream_t*)client, req->m_g, req->m_nKey, req->m_xKey);
            fus::crypt_stream_establish_client((fus::crypt_stream_t*)client, (fus::crypt_established_cb)_client_encrypted);
        } else {
            if (client->m_connectcb)
                client->m_connectcb(client, 0);
            client->m_proc(client);
        }
    }
}

static inline void _load_key(BIGNUM*& bn, const ST::string& key)
{
    bn = BN_new();
    uint8_t buf[64];
    FUS_ASSERTD(ST::base64_decode(key, buf, sizeof(buf)) == sizeof(buf));
    BN_bin2bn((unsigned char*)buf, sizeof(buf), bn);
}

void fus::client_crypt_connect(fus::client_t* client, const sockaddr* addr, const void* buf, size_t bufsz, client_connect_cb cb)
{
    FUS_ASSERTD(client);
    client->m_connectcb = cb;

    free(client->m_connectReq);
    client->m_connectReq = (connect_req_t*)malloc(sizeof(connect_req_t) + bufsz);
    _copy_sockaddr(client->m_connectReq, addr);
    client->m_connectReq->m_flags = connect_req_t::e_encrypt;
    client->m_connectReq->m_bufsz = bufsz;
    memcpy(client->m_connectReq->m_buf, buf, bufsz);

    uv_tcp_connect((uv_connect_t*)client->m_connectReq, (uv_tcp_t*)client, addr, (uv_connect_cb)_client_connected);
}

void fus::client_crypt_connect(fus::client_t* client, const sockaddr* addr, const void* buf, size_t bufsz,
                               uint32_t g, const ST::string& n, const ST::string& x, client_connect_cb cb)
{
    FUS_ASSERTD(client);
    client->m_connectcb = cb;

    free(client->m_connectReq);
    client->m_connectReq = (connect_req_t*)malloc(sizeof(connect_req_t) + bufsz);
    _copy_sockaddr(client->m_connectReq, addr);
    client->m_connectReq->m_flags = connect_req_t::e_encrypt | connect_req_t::e_ownsKeys;
    client->m_connectReq->m_g = g;
    // crypt_stream_t discards the keys when done with them. This lets higher level code reconnect
    // without having to fiddle with keys.
    _load_key(client->m_connectReq->m_nKey, n);
    _load_key(client->m_connectReq->m_xKey, x);
    client->m_connectReq->m_bufsz = bufsz;
    memcpy(client->m_connectReq->m_buf, buf, bufsz);

    uv_tcp_connect((uv_connect_t*)client->m_connectReq, (uv_tcp_t*)client, addr, (uv_connect_cb)_client_connected);
}

void fus::client_connect(fus::client_t* client, const sockaddr* addr, const void* buf, size_t bufsz, client_connect_cb cb)
{
    FUS_ASSERTD(client);
    client->m_connectcb = cb;

    free(client->m_connectReq);
    client->m_connectReq = (connect_req_t*)malloc(sizeof(connect_req_t) + bufsz);
    _copy_sockaddr(client->m_connectReq, addr);
    client->m_connectReq->m_flags = 0;
    client->m_connectReq->m_bufsz = bufsz;
    memcpy(client->m_connectReq->m_buf, buf, bufsz);

    uv_tcp_connect((uv_connect_t*)client->m_connectReq, (uv_tcp_t*)client, addr, (uv_connect_cb)_client_connected);
}

// =================================================================================

static void reconnect_client(uv_timer_t* timer)
{
    fus::client_t* client = (fus::client_t*)uv_handle_get_data((uv_handle_t*)timer);
    uv_tcp_connect((uv_connect_t*)client->m_connectReq, (uv_tcp_t*)client, (sockaddr*)&client->m_connectReq->m_sockaddr, (uv_connect_cb)_client_connected);
}

void fus::client_reconnect(fus::client_t* client, uint64_t reconnectTimeMs)
{
    FUS_ASSERTD(client);
    FUS_ASSERTD(client->m_connectReq);
    FUS_ASSERTD(!(((fus::tcp_stream_t*)client)->m_flags & tcp_stream_t::e_connected));
    uv_timer_start(&client->m_reconnect, reconnect_client, reconnectTimeMs, 0);
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
