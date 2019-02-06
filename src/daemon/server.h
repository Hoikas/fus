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

#include <filesystem>
#include <uv.h>

#include "core/config_parser.h"
#include "io/log_file.h"

namespace fus
{
    class console;

    class server
    {
        static server* m_instance;

        uv_tcp_t m_lobby;
        config_parser m_config;
        log_file m_log;
        uint32_t m_flags;

        struct admin_client_t* m_admin;

    protected:
        void admin_connect();
        bool admin_check(console&) const;
        bool admin_ping(console&, const ST::string&);

    protected:
        bool generate_keys(console&, const ST::string&);
        bool save_config(console&, const ST::string&);

    public:
        static server* get() { return m_instance; }

        server(const std::filesystem::path&);
        server(const server&) = delete;
        server(server&&) = delete;
        ~server();

        void start_console();
        bool start_lobby();
        void run_forever();
        void run_once();

    public:
        bool config2addr(const ST::string&, sockaddr_storage*);
        void fill_connection_header(void* packet);

    public:
        config_parser& config() { return m_config; }
        log_file& log() { return m_log; }

    public:
        void generate_client_ini(const std::filesystem::path& path) const;
        void generate_daemon_keys();
        void generate_daemon_keys(const ST::string&);
    };
};

#endif
