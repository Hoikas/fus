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

#ifndef __FUS_ADMIN_DAEMON_H
#define __FUS_ADMIN_DAEMON_H

#include "core/list.h"
#include "io/crypt_stream.h"

namespace fus
{
    struct admin_server_t;

    struct admin_server_t
    {
        crypt_stream_t m_stream;
        FUS_LIST_LINK(admin_server_t) m_link;
    };

    void admin_server_init(admin_server_t*);
    void admin_server_free(admin_server_t*);

    void admin_server_read(admin_server_t*);

    void admin_daemon_init();
    bool admin_daemon_running();
    void admin_daemon_free();

    void admin_daemon_accept(admin_server_t*, const void*);
};

#endif
