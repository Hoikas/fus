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

#ifndef __FUS_CLIENT_BASE_H
#define __FUS_CLIENT_BASE_H

#include "io/crypt_stream.h"
#include "io/net_error.h"
#include <map>

struct connect_req_t;

namespace fus
{
    struct client_t;
    typedef void (*client_connect_cb)(client_t*, ssize_t status);
    typedef void (*client_pump_proc)(client_t*);
    typedef void (*client_trans_cb)(void*, client_t*, uint32_t, net_error, ssize_t, const void*);
    typedef std::map<uint32_t, struct transaction_t> trans_map_t;

    struct transaction_t
    {
        void* m_instance;
        uint32_t m_transId;
        client_trans_cb m_cb;

        transaction_t(void* instance, uint32_t transId, client_trans_cb cb)
            : m_instance(instance), m_transId(transId), m_cb(cb)
        { }
    };

    struct client_t : public crypt_stream_t
    {
        client_connect_cb m_connectcb;
        client_pump_proc m_proc;

        uv_timer_t m_reconnect;
        ::connect_req_t* m_connectReq;

        uint32_t m_transId;
        trans_map_t m_trans;
    };

    int client_init(client_t*, uv_loop_t*);
    void client_free(client_t*);

    void client_crypt_connect(client_t*, const sockaddr*, const void*, size_t, client_connect_cb);
    void client_crypt_connect(client_t*, const sockaddr*, const void*, size_t, uint32_t, const ST::string&, const ST::string&, client_connect_cb);
    void client_connect(client_t*, const sockaddr*, const void*, size_t, client_connect_cb);
    void client_reconnect(client_t*, uint64_t reconnectTimeMs=30000);

    uint32_t client_next_transId(client_t*);
    uint32_t client_gen_trans(client_t*, void*, uint32_t, client_trans_cb);
    void client_fire_trans(client_t*, uint32_t, net_error, ssize_t, const void*);
    void client_kill_trans(client_t*, net_error, ssize_t, bool quiet=false);
    void client_kill_trans(client_t*, void*, net_error, ssize_t, bool quiet=false);

    template<typename _Msg>
    void client_prep_trans(client_t* client, _Msg& msg, void* instance, uint32_t wrapTransId,
                           client_trans_cb cb)
    {
        msg.set_transId(client_gen_trans(client, instance, wrapTransId, cb));
    }
};

#endif
