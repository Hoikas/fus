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

#ifndef __FUS_HASH_H
#define __FUS_HASH_H

#include <openssl/ossl_typ.h>
#include <string_theory/string>

namespace fus
{
    enum class hash_type
    {
        e_sha0,
        e_sha1,
        e_sha2,
    };

    class hash
    {
        EVP_MD_CTX* m_ctx;
        hash_type m_type;

    public:
        hash(hash_type type);
        ~hash();

        size_t digestsz() const;
        void hash_account(const ST::string& account, const ST::string& pass, void* hashbuf, size_t hashbufsz);
        void hash_login(const void* acctHash, size_t acctHashsz, uint32_t cliChallenge, uint32_t srvChallenge, void* hashbuf, size_t hashbufsz);
    };
};

#endif
