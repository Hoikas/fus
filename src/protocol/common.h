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

#ifndef __FUS_PROTOCOL_COMMON_H
#define __FUS_PROTOCOL_COMMON_H

#include "utils.h"

#include "protocol_fields_begin.inl"
#include "common.inl"
#include "protocol_fields_end.inl"

#include "protocol_structs_begin.inl"
#include "common.inl"
#include "protocol_structs_end.inl"

namespace fus
{
    namespace protocol
    {
        enum
        {
            // Externally facing services
            e_protocolCli2Auth = 0x0A,
            e_protocolCli2Game = 0x0B,
            e_protocolCli2File = 0x10,
            e_protocolCli2Gate = 0x16,

            // Externally facing services for admins
            e_protocolCli2Admin = 0x61,

            // Internal services
            e_protocolSrv2Master = 0x80,
            e_protocolSrv2Database = 0x81,
        };
    };
};

#endif
