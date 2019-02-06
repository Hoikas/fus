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

#ifndef __FUS_ADMIN_DAEMON_PRIVATE_H
#define __FUS_ADMIN_DAEMON_PRIVATE_H

#include "admin.h"
#include "core/list.h"
#include "daemon_base.h"
#include "io/log_file.h"

namespace fus
{
    struct admin_daemon_t
    {
        secure_daemon_t m_secure;
        log_file m_log;
        FUS_LIST_DECL(admin_server_t, m_link) m_clients;
    };
};

extern fus::admin_daemon_t* s_adminDaemon;

// =================================================================================

template<typename _Msg>
using _admin_cb = void(fus::admin_server_t*, ssize_t, _Msg*);

template<typename _Msg, typename _Cb=_admin_cb<_Msg>>
static inline void admin_read(fus::admin_server_t* client, _Cb cb)
{
    fus::tcp_stream_read_msg<_Msg>((fus::tcp_stream_t*)client, (fus::tcp_read_cb)cb);
}

// =================================================================================

#endif
