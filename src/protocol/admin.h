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

#ifndef __FUS_PROTOCOL_ADMIN_H
#define __FUS_PROTOCOL_ADMIN_H

namespace fus
{
    namespace protocol
    {
        namespace admin
        {
            enum
            {
                e_pingRequest,
                e_wallRequest,

                e_acctCreateRequest,
            };

            enum
            {
                e_pingReply,
                e_wallBCast,

                e_acctCreateReply,
            };
        };
    };
};

#include "utils.h"
#include "common.h"

#include "protocol_fields_begin.inl"
#include "admin.inl"
#include "protocol_fields_end.inl"

#include "protocol_warnings_silence.inl"
#include "protocol_structs_begin.inl"
#include "admin.inl"
#include "protocol_structs_end.inl"
#include "protocol_warnings_restore.inl"

#endif
