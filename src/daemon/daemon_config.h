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

#ifndef __FUS_DAEMON_CONFIG_H
#define __FUS_DAEMON_CONFIG_H

#include "core/config_parser.h"

namespace fus
{
    config_item daemon_config[] = {
        FUS_CONFIG_STR("lobby", "bindaddr", "127.0.0.1",
                       "Lobby Bind Address\n"
                       "IP Address that this fus server should listen for connections on")
        FUS_CONFIG_STR("lobby", "extaddr", "",
                       "External Address\n"
                       "IP Address that this server should communicate to clients.\n"
                       "This should be configured when NAT is in use.")
        FUS_CONFIG_INT("lobby", "port", 14617,
                       "Lobby Bind Port\n"
                       "Port that this fus server should listen for connections on")

        FUS_CONFIG_STR("log", "directory", "",
                       "Log Directory\n"
                       "Directory that the server's log files will be written to")
        FUS_CONFIG_STR("log", "level", "info",
                       "Log Level\n"
                       "Controls the verbosity of the server logs.\n"
                       "Possible Values:\n"
                       "    - debug: Highest level of verbosity.\n"
                       "    - info: Normal level of verbosity.\n"
                       "    - error: Lowest level of verbosity.")

        FUS_CONFIG_INT("client", "buildId", 918,
                       "Client Build ID\n"
                       "Build ID for clients connecting to this shard")
        FUS_CONFIG_INT("client", "branchId", 1,
                       "Client Branch ID\n"
                       "Branch ID for clients connecting to this shard")
        FUS_CONFIG_INT("client", "buildType", 50,
                       "Client Build Type\n"
                       "Build Type ID for clients connecting to this shard")
        FUS_CONFIG_STR("client", "product", "ea489821-6c35-4bd0-9dae-bb17c585e680",
                       "Client UUID\n"
                       "Product UUID for clients connecting to this shard")
        FUS_CONFIG_STR("client", "verification", "default",
                       "Client Verification Level\n"
                       "This specifies how closely the data provided in the client connection handshake should be verified.\n"
                       "Possible Values:\n"
                       "    - none: No verification, any client will be allowed to connect.\n"
                       "    - default: Default verification, any client can connect to file or gatekeeper but all others must match the expected values\n"
                       "    - strict: Strict verification, like default but the product uuid is verified for file and gatekeeper connections")

#define FUS_CONFIG_CLIENT(type) \
    FUS_CONFIG_STR(type, "addr", "", \
                   "Server Address\n" \
                   "Address to connect all " type " clients to") \
    FUS_CONFIG_INT(type, "port", 14617, \
                   "Server Port\n" \
                   "Port to connect all " type " clients to")

        FUS_CONFIG_CLIENT("admin")
        FUS_CONFIG_CLIENT("db")

#undef FUS_CONFIG_CLIENT

#define FUS_CONFIG_CRYPT(server, gval) \
    { ST_LITERAL("crypt"), ST_LITERAL(server "_k"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server "_n"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server "_x"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server "_g"), fus::config_item::value_type::e_integer, \
      ST_LITERAL(#gval), ST::null }, \

        FUS_CONFIG_CRYPT("admin", 19)
        FUS_CONFIG_CRYPT("auth", 41)
        FUS_CONFIG_CRYPT("db", 19)
        FUS_CONFIG_CRYPT("game", 73)
        FUS_CONFIG_CRYPT("gate", 4)

#undef FUS_CONFIG_CRYPT
    };
};

#endif
