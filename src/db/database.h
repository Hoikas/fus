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

#ifndef __FUS_DB_DATABASE_H
#define __FUS_DB_DATABASE_H

#include "core/uuid.h"
#include "db/constants.h"
#include "io/net_error.h"
#include <openssl/ossl_typ.h>
#include <string_theory/string>
#include <string_view>

namespace fus
{
    class config_parser;
    class log_file;

    typedef void (*database_cb)(void*, uint32_t, net_error);
    typedef void (*database_acct_auth_cb)(void*, uint32_t, net_error, const std::u16string_view&, const uuid&, uint32_t);
    typedef void (*database_acct_create_cb)(void*, uint32_t, net_error, const uuid&);

    namespace account_flags
    {
        enum
        {
            e_acctAdmin = (1<<0),
            e_acctBetaTester = (1<<2),
            e_acctBanned = (1<<16),
            e_acctMask = (e_acctAdmin | e_acctBanned | e_acctBetaTester),
        };
    };

    class database
    {
        static database* s_instance;

    protected:
        log_file* m_log;
        EVP_MD_CTX* m_hashCtx;

        database();
        database(const database&) = delete;
        database(database&&) = delete;

        virtual bool open(const fus::config_parser&) = 0;

    private:
        const EVP_MD* digest(hash_type hashType) const;

    protected:
        bool compare_hash(uint32_t cliChallenge, uint32_t srvChallenge, hash_type hashType,
                          const void* dbHash, size_t dbHashsz, const void* hashBuf, size_t hashBufsz);
        size_t digestsz(hash_type hashType) const;
        void hash_account(const std::u16string_view& name, const std::string_view& password, hash_type hashType,
                          void* outBuf, size_t outBufsz);

        void extract_hash(uint32_t& flags, hash_type& hashType) const;
        void stuff_hash(uint32_t& flags, hash_type hashType) const;

    public:
        virtual void authenticate_account(const std::u16string_view& name, uint32_t cliChallenge,
                                          uint32_t srvChallenge, hash_type hashType,
                                          const void* hashBuf, size_t hashBufsz,
                                          database_acct_auth_cb cb, void* instance,
                                          uint32_t transId) = 0;
        virtual void create_account(const std::u16string_view& name, const std::string_view& pass,
                                    uint32_t flags, database_acct_create_cb cb, void* instance,
                                    uint32_t transId) = 0;

    public:
        ~database();

        static database* init(const fus::config_parser&, fus::log_file&);
    };
};

#endif
