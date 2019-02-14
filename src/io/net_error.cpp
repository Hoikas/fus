/*   This file is part of fus.
 *
 *   fus is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   fus is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY, without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with fus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "net_error.h"

// =================================================================================

static ST::string s_errorStrings[] = {
    ST_LITERAL("Success"),
    ST_LITERAL("Internal Error"),
    ST_LITERAL("Timeout"),
    ST_LITERAL("Bad Server Data"),
    ST_LITERAL("Age Not Found"),
    ST_LITERAL("Connection Failed"),
    ST_LITERAL("Disconnected"),
    ST_LITERAL("File Not Found"),
    ST_LITERAL("Old Build"),
    ST_LITERAL("Remote Shutdown"),
    ST_LITERAL("ODBC Timeout"),
    ST_LITERAL("Account Already Exists"),
    ST_LITERAL("Player Already Exists"),
    ST_LITERAL("Account Not Found"),
    ST_LITERAL("Player Not Found"),
    ST_LITERAL("Invalid Parameter"),
    ST_LITERAL("Name Lookup Failed"),
    ST_LITERAL("Logged in Elsewhere"),
    ST_LITERAL("Vault Node Not Found"),
    ST_LITERAL("Too Many Players on Account"),
    ST_LITERAL("Authentication Failed"),
    ST_LITERAL("State Object Not Found"),
    ST_LITERAL("Login Denied"),
    ST_LITERAL("Circular Reference"),
    ST_LITERAL("Account Not Activated"),
    ST_LITERAL("Key Already Used"),
    ST_LITERAL("Key Not Found"),
    ST_LITERAL("Activation Code Not Found"),
    ST_LITERAL("Player Name Invalid"),
    ST_LITERAL("Not Supported"),
    ST_LITERAL("Service Forbidden"),
    ST_LITERAL("Auth Token Too Old"),
    ST_LITERAL("Must Use GameTap Client"), //  ORLY?
    ST_LITERAL("Too Many Failed Logins"),
    ST_LITERAL("GameTap Connection Failed"),
    ST_LITERAL("GameTap Too Many Auth Options"),
    ST_LITERAL("GameTap Missing Parameter"),
    ST_LITERAL("GameTap Server Error"),
    ST_LITERAL("Account Banned"),
    ST_LITERAL("Kicked by CCR"),
    ST_LITERAL("Score Wrong Type"),
    ST_LITERAL("Score Not Enough Points"),
    ST_LITERAL("Score Already Exists"),
    ST_LITERAL("Score No Data Found"),
    ST_LITERAL("Invite No Matching Player"),
    ST_LITERAL("Invite Too Many Neighborhoods"),
    ST_LITERAL("Need to Pay"),
    ST_LITERAL("Server Busy"),
    ST_LITERAL("Vault Node Access Violation"),
};

template <typename _T, size_t _Sz>
static constexpr size_t arrsize(_T(&)[_Sz]) { return _Sz; }
static_assert(arrsize(s_errorStrings) == (size_t)fus::net_error::e_numErrors);

// =================================================================================

const ST::string& fus::net_error_string(fus::net_error error)
{
    if (error < net_error::e_numErrors)
        return s_errorStrings[(size_t)error];

    static const ST::string pending = ST_LITERAL("Pending");
    return pending;
}
