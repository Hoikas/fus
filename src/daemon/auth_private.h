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
#include "daemon_base.h"
#include "io/log_file.h"

namespace fus
{
    struct auth_daemon_t
    {
        secure_daemon_t m_secure;
        log_file m_log;
    };
};

extern fus::auth_daemon_t* s_authDaemon;

// =================================================================================

template<typename _Msg>
using _auth_cb = void(fus::auth_client_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_auth_cb<_Msg>>
static inline void auth_read(fus::auth_client_t* client, _Cb cb)
{
    fus::crypt_stream_read_msg<_Msg>((fus::crypt_stream_t*)client, (fus::tcp_read_cb)cb);
}

// =================================================================================

enum
{
    e_clientRegistered = (1<<0),
};

#endif
