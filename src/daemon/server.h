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
#include <string_theory/string>
#include <unordered_map>
#include <uv.h>

#include "core/config_parser.h"
#include "io/log_file.h"

namespace fus
{
    class console;

    typedef void (*daemon_ctl_noresult_f)();
    typedef bool (*daemon_ctl_result_f)();
    struct daemon_ctl_t
    {
        daemon_ctl_result_f init;
        daemon_ctl_noresult_f shutdown;
        daemon_ctl_noresult_f free;
        daemon_ctl_result_f running;
        daemon_ctl_result_f shutting_down;
        bool m_enabled;

        daemon_ctl_t(daemon_ctl_result_f i, daemon_ctl_noresult_f sdi, daemon_ctl_noresult_f f,
                     daemon_ctl_result_f r, daemon_ctl_result_f sdq, bool e)
            : init(i), shutdown(sdi), free(f), running(r), shutting_down(sdq), m_enabled(e)
        { }
    };
    typedef std::unordered_map<ST::string, daemon_ctl_t, ST::hash_i, ST::equal_i> daemon_ctl_map_t;

    class server
    {
        static server* m_instance;

        enum
        {
            e_lobbyReady = (1<<0),
            e_running = (1<<1),
            e_shuttingDown = (1<<2),
            e_hasShutdownTimer = (1<<3),
        };

        uv_tcp_t m_lobby;
        uv_timer_t m_shutdown;
        config_parser m_config;
        log_file m_log;
        uint32_t m_flags;

        struct admin_client_t* m_admin;
        daemon_ctl_map_t m_daemonCtl;
        std::vector<daemon_ctl_map_t::iterator> m_daemonIts;

    protected:
        static void admin_disconnected(fus::admin_client_t*);
        void admin_init();
        bool admin_check(console&) const;
        bool admin_acctCreate(console&, const ST::string&);
        bool admin_ping(console&, const ST::string&);
        bool admin_wall(console&, const ST::string&);

    protected:
        bool daemon_ctl(console&, const ST::string&);
        bool generate_keys(console&, const ST::string&);
        bool quit(console&, const ST::string&);
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

    protected:
        bool init_daemons();
        void free_daemons();
        void shutdown();
        static void force_shutdown(uv_timer_t*);

    protected:
        void daemon_ctl_noresult(const ST::string& action, const ST::string& daemon,
                                 daemon_ctl_noresult_f proc, const ST::string& success);
        bool daemon_ctl_result(const ST::string& action, const ST::string& daemon,
                                 daemon_ctl_result_f proc, const ST::string& success,
                                 const ST::string& fail);

        template<size_t _ActionSz, size_t _SuccessSz>
        void daemon_ctl_noresult(const char(&action)[_ActionSz], const ST::string& daemon,
                                 daemon_ctl_noresult_f proc, const char(&success)[_SuccessSz])
        {
            daemon_ctl_noresult(ST::string::from_literal(action, _ActionSz-1), daemon, proc,
                                ST::string::from_literal(success, _SuccessSz-1));
        }

        template<size_t _ActionSz, size_t _SuccessSz, size_t _FailSz>
        bool daemon_ctl_result(const char(&action)[_ActionSz], const ST::string& daemon,
                                 daemon_ctl_result_f proc, const char(&success)[_SuccessSz],
                                 const char(&fail)[_FailSz])
        {
            return daemon_ctl_result(ST::string::from_literal(action, _ActionSz-1), daemon, proc,
                                     ST::string::from_literal(success, _SuccessSz-1),
                                     ST::string::from_literal(fail, _FailSz-1));
        }

    public:
        bool config2addr(const ST::string&, sockaddr_storage*);
        void fill_connection_header(void* packet);

    public:
        config_parser& config() { return m_config; }
        log_file& log() { return m_log; }

    public:
        void generate_client_ini(const std::filesystem::path& path) const;
        void generate_daemon_keys(bool quiet=false);
        void generate_daemon_keys(const ST::string&, bool quiet=false);
    };
};

#endif
