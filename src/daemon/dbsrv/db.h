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

#ifndef __FUS_DB_DAEMON_H
#define __FUS_DB_DAEMON_H

#include "core/list.h"
#include "io/crypt_stream.h"

namespace fus
{
    struct db_server_t;

    struct db_server_t : public crypt_stream_t
    {
        FUS_LIST_LINK(db_server_t) m_link;
    };

    void db_server_init(db_server_t*);
    void db_server_free(db_server_t*);

    void db_server_read(db_server_t*);

    bool db_daemon_init();
    bool db_daemon_running();
    bool db_daemon_shutting_down();
    void db_daemon_free();
    void db_daemon_shutdown();

    void db_daemon_accept(db_server_t*, const void*);
};

#endif
