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

#ifndef __FUS_SERVER_H
#define __FUS_SERVER_H

#include <uv.h>

#include "config_parser.h"

namespace fus
{
    class server
    {
        static server* m_instance;

        uv_tcp_t m_lobby;
        config_parser m_config;
        uint32_t m_flags;

    public:
        static server* get() { return m_instance; }

        server(const std::filesystem::path&);
        server(const server&) = delete;
        server(server&&) = delete;
        ~server();

        config_parser& config() { return m_config; }

        bool start_lobby();
        void run_forever();
        void run_once();
    };
};

#endif
