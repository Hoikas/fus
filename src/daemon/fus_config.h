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

#include "config_parser.h"

#define FUS_CONFIG_CRYPT(server, gval) \
    { ST_LITERAL("crypt"), ST_LITERAL(server##"_k"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server##"_n"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server##"_x"), fus::config_item::value_type::e_string, \
      ST::null, ST::null }, \
    { ST_LITERAL("crypt"), ST_LITERAL(server##"_g"), fus::config_item::value_type::e_integer, \
      ST_LITERAL(#gval), ST::null }, \

namespace fus
{
    config_item daemon_config[] = {
        FUS_CONFIG_STR("lobby", "bindaddr", "127.0.0.1",
                       "Lobby Bind Address\n"
                       "IPv4 Address that this fus server should listen for connections on")
        FUS_CONFIG_INT("lobby", "port", 14617,
                       "Lobby Bind Port\n"
                       "Port that this fus server should listen for connections on")

        FUS_CONFIG_CRYPT("auth", 41)
        FUS_CONFIG_CRYPT("game", 73)
        FUS_CONFIG_CRYPT("gate", 4)
    };
};

#undef FUS_CONFIG_CRYPT

#endif
