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

#ifndef __FUS_AUTH_DAEMON_PRIVATE_H
#define __FUS_AUTH_DAEMON_PRIVATE_H

#include "auth.h"
#include "daemon/daemon_base.h"

namespace fus
{
    struct auth_daemon_t : public db_trans_daemon_t
    {
        FUS_LIST_DECL(auth_server_t, m_link) m_clients;
    };
};

extern fus::auth_daemon_t* s_authDaemon;

// =================================================================================

template<typename _Msg>
using _auth_cb = void(fus::auth_server_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_auth_cb<_Msg>>
static inline void auth_read(fus::auth_server_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>(client, (fus::tcp_read_cb)cb);
}

// =================================================================================

enum
{
    e_clientRegistered = (1<<0),
    e_acctLoggedIn = (1<<1),
    e_acctLoginInProgress = (1<<2),
};

#endif
