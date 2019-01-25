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

#include "io/crypt_stream.h"

namespace fus
{
    struct secure_daemon_t;

    struct secure_daemon_t
    {
        BIGNUM* m_bnK;
        BIGNUM* m_bnN;
    };

    void secure_daemon_init(secure_daemon_t*, const ST::string&);
    void secure_daemon_close(secure_daemon_t*);
    void secure_daemon_encrypt_stream(secure_daemon_t*, crypt_stream_t*, crypt_established_cb=nullptr);
};

#endif
