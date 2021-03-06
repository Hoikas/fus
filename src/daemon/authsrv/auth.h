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

#ifndef __FUS_AUTH_DAEMON_H
#define __FUS_AUTH_DAEMON_H

#include "core/list.h"
#include "io/crypt_stream.h"

namespace fus
{
    struct auth_server_t;
    class log_file;

    struct auth_server_t : public crypt_stream_t
    {
        FUS_LIST_LINK(auth_server_t) m_link;

        uint32_t m_flags;
        uint32_t m_srvChallenge;
    };

    void auth_server_init(auth_server_t*);
    void auth_server_free(auth_server_t*);

    void auth_server_read(auth_server_t*);

    bool auth_daemon_init();
    bool auth_daemon_running();
    bool auth_daemon_shutting_down();
    void auth_daemon_free();
    void auth_daemon_shutdown();

    void auth_daemon_accept(auth_server_t*, const void*);
};

#endif
