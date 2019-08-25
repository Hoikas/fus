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

#ifndef __FUS_DAEMON_BASE
#define __FUS_DAEMON_BASE

#include <openssl/ossl_typ.h>
#include <string_theory/string>

#include "core/uuid.h"
#include "io/crypt_stream.h"
#include "io/log_file.h"

namespace fus
{
    struct db_client_t;
    class log_file;

    enum class client_verification
    {
        e_none,
        e_default,
        e_strict,
    };

    struct daemon_t
    {
        enum
        {
            e_shuttingDown = (1<<0),
            e_dbConnected = (1<<1),
        };

        uint32_t m_buildId;
        uint32_t m_branchId;
        uint32_t m_buildType;
        fus::uuid m_product;
        client_verification m_verification;
        log_file m_log;
        uint32_t m_flags;
    };

    void daemon_init(daemon_t*, const ST::string&);
    void daemon_free(daemon_t*);
    void daemon_shutdown(daemon_t*);
    bool daemon_verify_connection(const daemon_t*, const void*, bool);

    struct secure_daemon_t : public daemon_t
    {
        BIGNUM* m_bnK;
        BIGNUM* m_bnN;
    };

    void secure_daemon_init(secure_daemon_t*, const ST::string&);
    void secure_daemon_free(secure_daemon_t*);
    void secure_daemon_encrypt_stream(secure_daemon_t*, crypt_stream_t*, crypt_established_cb=nullptr);
    void secure_daemon_shutdown(secure_daemon_t*);

    struct db_trans_daemon_t : public secure_daemon_t
    {
        db_client_t* m_db;
    };

    void db_trans_daemon_init(db_trans_daemon_t*, const ST::string&);
    void db_trans_daemon_free(db_trans_daemon_t*);
    void db_trans_daemon_shutdown(db_trans_daemon_t*);
};

#endif
