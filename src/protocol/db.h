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

#ifndef __FUS_PROTOCOL_DB_H
#define __FUS_PROTOCOL_DB_H

#include "utils.h"
#include "common.h"

#include "protocol_fields_begin.inl"
#include "db.inl"
#include "protocol_fields_end.inl"

#include "protocol_warnings_silence.inl"
#include "protocol_structs_begin.inl"
#include "db.inl"
#include "protocol_structs_end.inl"
#include "protocol_warnings_restore.inl"

namespace fus
{
    namespace protocol
    {
#pragma pack(push,1)
        template<typename _Msg>
        struct db_msg
        {
            msg_std_header m_header;
            _Msg m_contents;

            operator void*() { return (void*)(&m_header); }
        };
#pragma pack(pop)

        namespace client2db
        {
            enum
            {
                e_pingRequest,

                e_acctCreateRequest,

                e_numMsgs
            };
        };

        namespace db2client
        {
            enum
            {
                e_pingReply,

                e_acctCreateReply,

                e_numMsgs
            };
        };
    };
};

#endif
