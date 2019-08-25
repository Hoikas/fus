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

#ifndef __FUS_ADMIN_CLIENT_H
#define __FUS_ADMIN_CLIENT_H

#include "client_base.h"

namespace fus
{
    struct admin_client_t;
    typedef void (*admin_client_wall_cb)(admin_client_t*, const ST::string&, const ST::string&);

    struct admin_client_t : public client_t
    {
        admin_client_wall_cb m_wallcb;
    };

    int admin_client_init(admin_client_t*, uv_loop_t*);
    void admin_client_free(admin_client_t*);

    void admin_client_connect(admin_client_t*, const sockaddr*, void*, size_t, client_connect_cb);
    void admin_client_connect(admin_client_t*, const sockaddr*, void*, size_t, uint32_t, const ST::string&, const ST::string&, client_connect_cb);
    size_t admin_client_header_size();

    void admin_client_wall_handler(admin_client_t*, admin_client_wall_cb cb=nullptr);

    void admin_client_read(admin_client_t*);
};

#endif
