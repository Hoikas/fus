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

#ifndef __FUS_DB_CLIENT_H
#define __FUS_DB_CLIENT_H

#include "client_base.h"

namespace fus
{
    enum class hash_type;
    class uuid;

    struct db_client_t;

    struct db_client_t : public client_t
    {
    };

    int db_client_init(db_client_t*, uv_loop_t*);
    void db_client_free(db_client_t*);

    void db_client_connect(db_client_t*, const sockaddr*, void*, size_t, client_connect_cb);
    void db_client_connect(db_client_t*, const sockaddr*, void*, size_t, uint32_t, const ST::string&, const ST::string&, client_connect_cb);
    size_t db_client_header_size();


    void db_client_ping(db_client_t*, uint32_t, client_trans_cb cb=nullptr, void* instance=nullptr, uint32_t transId=0);
    void db_client_authenticate_account(db_client_t*, const ST::string&, uint32_t, uint32_t, hash_type, const void*, size_t, client_trans_cb cb=nullptr, void* instance=nullptr, uint32_t transId=0);
    void db_client_create_account(db_client_t*, const ST::string&, const ST::string&, uint32_t, client_trans_cb cb=nullptr, void* instance=nullptr, uint32_t transId=0);

    void db_client_read(db_client_t*);
};

#endif
