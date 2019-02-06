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

#ifndef __FUS_PROTOCOL_AUTH_H
#define __FUS_PROTOCOL_AUTH_H

#include "utils.h"
#include "common.h"

#include "protocol_fields_begin.inl"
#include "auth.inl"
#include "protocol_fields_end.inl"

#include "protocol_warnings_silence.inl"
#include "protocol_structs_begin.inl"
#include "auth.inl"
#include "protocol_structs_end.inl"
#include "protocol_warnings_restore.inl"

namespace fus
{
    namespace protocol
    {
#pragma pack(push,1)
        template<typename _Msg>
        struct auth_msg
        {
            msg_std_header m_header;
            _Msg m_contents;

            operator void*() { return (void*)(&m_header); }
        };
#pragma pack(pop)

        namespace client2auth
        {
            enum
            {
                // Global
                e_pingRequest,

                // Client
                e_clientRegisterRequest,
                e_clientSetCCRLevel,

                // Account
                e_acctLoginRequest,
                e_acctSetEulaVersion,
                e_acctSetDataRequest,
                e_acctSetPlayerRequest,
                e_acctCreateRequest,
                e_acctChangePasswordRequest,
                e_acctSetRolesRequest,
                e_acctSetBillingTypeRequest,
                e_acctActivateRequest,
                e_acctCreateFromKeyRequest,

                // Player
                e_playerDeleteRequest,
                e_playerUndeleteRequest,
                e_playerSelectRequest,
                e_playerRenameRequest,
                e_playerCreateRequest,
                e_playerSetStatus,
                e_playerChat,
                e_upgradeVisitorRequest,
                e_setPlayerBanStatusRequest,
                e_kickPlayer,
                e_changePlayerNameRequest,
                e_sendFriendInviteRequest,

                // Vault
                e_vaultNodeCreate,
                e_vaultNodeFetch,
                e_vaultNodeSave,
                e_vaultNodeDelete,
                e_vaultNodeAdd,
                e_vaultNodeRemove,
                e_vaultFetchNodeRefs,
                e_vaultInitAgeRequest,
                e_vaultNodeFind,
                e_vaultSetSeen,
                e_vaultSendNode,

                // Ages
                e_ageRequest,

                // File-related
                e_fileListRequest,
                e_fileDownloadRequest,
                e_fileDownloadChunkAck,

                // Game
                e_propagateBuffer,

                // Public ages
                e_getPublicAgeList,
                e_setAgePublic,

                // Log Messages
                e_logPythonTraceback,
                e_logStackDump,
                e_logClientDebuggerConnect,

                // Score
                e_scoreCreate,
                e_scoreDelete,
                e_scoreGetScores,
                e_scoreAddPoints,
                e_scoreTransferPoints,
                e_scoreSetPoints,
                e_scoreGetRanks,

                e_accountExistsRequest,

                // Extension messages
                e_ageRequestEx = 0x1000,
                e_scoreGetHighScores,

                e_numMsgs
            };
        };

        namespace auth2client
        {
            enum
            {
                // Global
                e_pingReply,
                e_serverAddr,
                e_notifyNewBuild,

                // Client
                e_clientRegisterReply,

                // Account
                e_acctLoginReply,
                e_acctData,
                e_acctPlayerInfo,
                e_acctSetPlayerReply,
                e_acctCreateReply,
                e_acctChangePasswordReply,
                e_acctSetRolesReply,
                e_acctSetBillingTypeReply,
                e_acctActivateReply,
                e_acctCreateFromKeyReply,

                // Player
                e_playerList,
                e_playerChat,
                e_playerCreateReply,
                e_playerDeleteReply,
                e_upgradeVisitorReply,
                e_setPlayerBanStatusReply,
                e_changePlayerNameReply,
                e_sendFriendInviteReply,

                // Friends
                e_friendNotify,

                // Vault
                e_vaultNodeCreated,
                e_vaultNodeFetched,
                e_vaultNodeChanged,
                e_vaultNodeDeleted,
                e_vaultNodeAdded,
                e_vaultNodeRemoved,
                e_vaultNodeRefsFetched,
                e_vaultInitAgeReply,
                e_vaultNodeFindReply,
                e_vaultSaveNodeReply,
                e_vaultAddNodeReply,
                e_vaultRemoveNodeReply,

                // Ages
                e_ageReply,

                // File-related
                e_fileListReply,
                e_fileDownloadChunk,

                // Game
                e_propagateBuffer,

                // Admin
                e_kickedOff,

                // Public ages
                e_publicAgeList,

                // Score
                e_scoreCreateReply,
                e_scoreDeleteReply,
                e_scoreGetScoresReply,
                e_scoreAddPointsReply,
                e_scoreTransferPointsReply,
                e_scoreSetPointsReply,
                e_scoreGetRanksReply,

                e_accountExistsReply,

                // Extension messages
                e_ageReplyEx = 0x1000,
                e_scoreGetHighScoresReply,
                e_serverCaps,

                e_numMsgs
            };
        };
    };
};

#endif
