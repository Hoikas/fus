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
#include "io/net_error.h"
#include <string_theory/string>

namespace fus
{
    class config_parser;
    class log_file;

    typedef void (*database_cb)(void*, uint32_t, net_error);
    typedef void (*database_acct_auth_cb)(void*, uint32_t, net_error, const void*, size_t, const uuid&, uint32_t);
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

        database() { }
        database(const database&) = delete;
        database(database&&) = delete;

        virtual bool open(const fus::config_parser&) = 0;

    public:
        virtual void authenticate_account(const ST::string& name, const void* hashBuf,
                                          size_t hashBufsz, const void* saltBuf, size_t saltBufsz,
                                          database_acct_auth_cb cb, void* instance, uint32_t transId) = 0;
        virtual void create_account(const ST::string& name, const void* hashBuf, size_t hashBufsz,
                                    uint32_t flags, database_acct_create_cb cb, void* instance, uint32_t transId) = 0;

    public:
        static database* init(const fus::config_parser&, fus::log_file&);
    };
};

#endif
