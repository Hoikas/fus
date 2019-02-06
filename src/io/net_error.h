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

#ifndef __FUS_NET_ERROR_H
#define __FUS_NET_ERROR_H

#include <cstdint>

namespace fus
{
    enum class net_error : uint32_t
    {
        e_pending                  = UINT32_MAX,
        e_success                  =  0,

        e_internalError            =  1,
        e_timeout                  =  2,
        e_badServerData            =  3,
        e_ageNotFound              =  4,
        e_connectFailed            =  5,
        e_disconnected             =  6,
        e_fileNotFound             =  7,
        e_oldBuildId               =  8,
        e_remoteShutdown           =  9,
        e_timeoutOdbc              = 10,
        e_accountAlreadyExists     = 11,
        e_playerAlreadyExists      = 12,
        e_accountNotFound          = 13,
        e_playerNotFound           = 14,
        e_invalidParameter         = 15,
        e_nameLookupFailed         = 16,
        e_loggedInElsewhere        = 17,
        e_vaultNodeNotFound        = 18,
        e_maxPlayersOnAcct         = 19,
        e_authenticationFailed     = 20,
        e_stateObjectNotFound      = 21,
        e_loginDenied              = 22,
        e_circularReference        = 23,
        e_accountNotActivated      = 24,
        e_keyAlreadyUsed           = 25,
        e_keyNotFound              = 26,
        e_activationCodeNotFound   = 27,
        e_playerNameInvalid        = 28,
        e_notSupported             = 29,
        e_serviceForbidden         = 30,
        e_authTokenTooOld          = 31,
        e_mustUseGameTapClient     = 32,
        e_tooManyFailedLogins      = 33,
        e_gameTapConnectionFailed  = 34,
        e_GTTooManyAuthOptions     = 35,
        e_GTMissingParameter       = 36,
        e_GTServerError            = 37,
        e_accountBanned            = 38,
        e_kickedByCCR              = 39,
        e_scoreWrongType           = 40,
        e_scoreNotEnoughPoints     = 41,
        e_scoreAlreadyExists       = 42,
        e_scoreNoDataFound         = 43,
        e_inviteNoMatchingPlayer   = 44,
        e_inviteTooManyHoods       = 45,
        e_needToPay                = 46,
        e_serverBusy               = 47,
        e_vaultNodeAccessViolation = 48,

        e_numErrors
    };
};

#endif
