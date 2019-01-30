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

#include "io/crypt_stream.h"

namespace fus
{
    struct auth_client_t;

    struct auth_client_t
    {
        crypt_stream_t m_stream;
        uint32_t m_flags;
        uint32_t m_loginSalt;
    };

    void auth_client_init(auth_client_t*);
    void auth_client_free(auth_client_t*);
    void auth_client_shutdown(auth_client_t*);

    void auth_client_read(auth_client_t*);

    void auth_daemon_init();
    bool auth_daemon_running();
    void auth_daemon_close();

    void auth_daemon_accept_client(auth_client_t*, const void*);
};

#endif
