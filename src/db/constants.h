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

#ifndef __FUS_DB_CONSTANTS
#define __FUS_DB_CONSTANTS

namespace fus
{
    namespace acct_flags
    {
        enum
        {
            // These roles are defined by Plasma
            e_roleDisabled = 0<<0,
            e_roleAdmin = 1<<0,
            e_roleDeveloper = 1<<1,
            e_roleBetaTester = 1<<2,
            e_roleUser = 1<<3,
            e_roleSpecialEvent = 1<<4,
            e_roleBanned = 1<<16,

            // internal fus flags
            e_acctHashSha0 = 1<<17,
            e_acctHashSha1 = 1<<18,
            e_acctHashMask = e_acctHashSha0 | e_acctHashSha1,
        };
    };

    enum class hash_type
    {
        e_unspecified,
        e_sha0,
        e_sha1,
    };
};

#endif
