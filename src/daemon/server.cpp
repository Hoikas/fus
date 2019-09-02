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

#include <iostream>

#include "adminsrv/admin.h"
#include "authsrv/auth.h"
#include "client/admin_client.h"
#include "core/errors.h"
#include "daemon_config.h"
#include "dbsrv/db.h"
#include <gflags/gflags.h>
#include "io/console.h"
#include "io/io.h"
#include "io/net_struct.h"
#include "io/tcp_stream.h"
#include "protocol/common.h"
#include "server.h"

// =================================================================================

DEFINE_bool(run_admin, true, "Launch the admin daemon");
DEFINE_bool(run_auth, true, "Launch the auth daemon");
DEFINE_bool(run_db, true, "Launch the database daemon");

// =================================================================================

fus::server* fus::server::m_instance = nullptr;

// =================================================================================

template<typename... _Args>
struct max_sizeof
{
    static constexpr size_t value = 0;
};

template<typename _Arg0, typename ..._Args>
struct max_sizeof<_Arg0, _Args...>
{
    static constexpr size_t value = std::max(sizeof(_Arg0), max_sizeof<_Args...>::value);
};

constexpr size_t k_clientMemsz = max_sizeof<fus::admin_server_t, fus::auth_server_t>::value;

// =================================================================================

fus::server::server(const std::filesystem::path& config_path)
    : m_config(fus::daemon_config), m_flags(), m_admin(nullptr)
{
    m_instance = this;
    m_config.read(config_path);

    log_file::set_directory(m_config.get<const ST::string&>("log", "directory"));
    m_log.set_level(m_config.get<const ST::string&>("log", "level"));

#define ADD_DAEMON(name) \
    { \
    auto pair = m_daemonCtl.emplace(std::piecewise_construct, std::forward_as_tuple(ST_LITERAL(#name)), \
                                    std::forward_as_tuple(name##_daemon_init, name##_daemon_shutdown, \
                                                          name##_daemon_free, name##_daemon_running, \
                                                          name##_daemon_shutting_down, FLAGS_run_##name)); \
    m_daemonIts.push_back(pair.first); \
    }

    // All daemons must be entered into the map in the order that they should be inited
    // Hint: if another server connects to it, init it first... May GAWD help you if you're trying
    //       something crazy, like circular connections.
    ADD_DAEMON(db);
    ADD_DAEMON(admin);
    ADD_DAEMON(auth);

#undef ADD_DAEMON
}

fus::server::~server()
{
    m_instance = nullptr;
    free_daemons();
    console::get().end(); // idempotent
    m_log.close();
}

// =================================================================================

static void _on_header_read(fus::tcp_stream_t* client, ssize_t error, void* msg)
{
    fus::log_file& log = fus::server::get()->log();
    if (error < 0) {
        log.write_debug("[{}] Connection Header read error: {}", fus::tcp_stream_peeraddr(client), uv_strerror(error));
        fus::tcp_stream_shutdown(client);
        return;
    }

    fus::protocol::common_connection_header* header = (fus::protocol::common_connection_header*)msg;
    switch (header->get_connType()) {
    case fus::protocol::e_protocolCli2Admin:
        log.write_debug("[{}] Incoming admin connection", fus::tcp_stream_peeraddr(client));
        fus::admin_daemon_accept((fus::admin_server_t*)client, msg);
        break;
    case fus::protocol::e_protocolCli2Auth:
        log.write_debug("[{}] Incoming auth connection", fus::tcp_stream_peeraddr(client));
        fus::auth_daemon_accept((fus::auth_server_t*)client, msg);
        break;
    case fus::protocol::e_protocolSrv2Database:
        log.write_debug("[{}] Incoming db connection", fus::tcp_stream_peeraddr(client));
        fus::db_daemon_accept((fus::db_server_t*)client, msg);
        break;
    default:
        log.write_error("[{}] Invalid connection type '{2X}'", fus::tcp_stream_peeraddr(client), header->get_connType());
        fus::tcp_stream_shutdown(client);
        break;
    }
}

static void _on_client_connect(fus::tcp_stream_t* lobby, int status)
{
    if (status < 0) {
        fus::server::get()->log().write_debug("New connection error: {}", uv_strerror(status));
        return;
    }

    // We absolutely must allocate enough space for the largest client type. It would be nice
    // if we could realloc() when we knew what the connection type is, but that could result in
    // the pointer address changing. That's a generally a bad thing for non-POD, like us.
    fus::tcp_stream_t* client = (fus::tcp_stream_t*)malloc(k_clientMemsz);
    fus::tcp_stream_init(client, uv_default_loop());
    fus::tcp_stream_free_on_close(client, true);
    if (fus::tcp_stream_accept(lobby, client) == 0) {
        fus::tcp_stream_read_msg<fus::protocol::common_connection_header>(client, _on_header_read);
    } else {
        uv_close((uv_handle_t*)client, (uv_close_cb)fus::tcp_stream_free);
    }
}

bool fus::server::start_lobby()
{
    FUS_ASSERTD(!(m_flags & e_lobbyReady));
    FUS_ASSERTD(!(m_flags & e_running));

    uv_loop_t* loop = uv_default_loop();
    m_log.open(loop, ST_LITERAL("lobby"));

    const char* bindaddr = m_config.get<const char*>("lobby", "bindaddr");
    int port = m_config.get<int>("lobby", "port");
    m_log.write_info("Binding to '{}/{}'", bindaddr, port);

    uv_tcp_init(loop, &m_lobby);
    sockaddr_storage addr;
    FUS_ASSERTD(str2addr(bindaddr, port, &addr));
    if (uv_tcp_bind(&m_lobby, (sockaddr*)&addr, 0) < 0) {
        m_log.write_error("Failed to bind to '{}/{}'", bindaddr, port);
        return false;
    }
    if (uv_listen((uv_stream_t*)&m_lobby, 128, (uv_connection_cb)_on_client_connect) < 0) {
        m_log.write_error("Failed to listen for incoming connections on '{}/{}'", bindaddr, port);
        return false;
    }
    m_flags |= e_lobbyReady;

    if (!init_daemons()) {
        shutdown();
        return false;
    }
    return true;
}

// =================================================================================

void fus::server::run_forever()
{
    FUS_ASSERTD(!(m_flags & e_running));

    m_flags |= e_running;
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    m_flags &= ~e_running;
}

void fus::server::run_once()
{
    FUS_ASSERTD(!(m_flags & e_running));

    m_flags |= e_running;
    uv_run(uv_default_loop(), UV_RUN_ONCE);
    m_flags &= ~e_running;
}

// =================================================================================

bool fus::server::init_daemons()
{
    for (auto it = m_daemonIts.begin(); it != m_daemonIts.end(); ++it) {
        if ((*it)->second.m_enabled)
            if (!daemon_ctl_result("Starting", (*it)->first, (*it)->second.init, "[  OK  ]", "[FAILED]"))
                return false;
    }
    return true;
}

void fus::server::free_daemons()
{
    for (auto it = m_daemonIts.rbegin(); it != m_daemonIts.rend(); ++it) {
        // We're at the point where console output is irrelevant
        if ((*it)->second.running())
            (*it)->second.free();
    }
}

void fus::server::shutdown()
{
    m_flags |= e_shuttingDown;

    // First attempt: be a nice guy and request for everyone to shutdown.
    console::get().end();
    if (m_admin) {
        fus::tcp_stream_free_on_close(m_admin, true);
        fus::tcp_stream_shutdown(m_admin);
    }

    for (auto it = m_daemonIts.rbegin(); it != m_daemonIts.rend(); ++it) {
        if ((*it)->second.running())
            daemon_ctl_noresult("Shutting down", (*it)->first, (*it)->second.shutdown, "[  OK  ]");
    }
    uv_close((uv_handle_t*)&m_lobby, nullptr);

    // The "nice" shutdown may take a few loop iterations, so we'll wait nicely for a bit.
    console::get() << console::weight_bold << console::foreground_yellow
                   << "Waiting for shutdown to complete..." << console::endl;
    m_flags |= e_hasShutdownTimer;
    uv_timer_init(uv_default_loop(), &m_shutdown);
    uv_handle_set_data((uv_handle_t*)&m_shutdown, this);
    uv_timer_start(&m_shutdown, force_shutdown, 5000, 0);
    uv_unref((uv_handle_t*)&m_shutdown);
}

void fus::server::force_shutdown(uv_timer_t* timer)
{
    // https://www.youtube.com/watch?v=ujBjn1qeCLs
    console::get() << console::weight_bold << console::foreground_red
                   << "Shutdown has potentially hung... Forcibly terminating local fus daemon."
                   << console::endl;

    fus::server* server = (fus::server*)uv_handle_get_data((uv_handle_t*)timer);

    // Forcibly closes all handles and the loop
    uv_walk(uv_default_loop(), (uv_walk_cb)uv_close, nullptr);
    server->m_flags &= ~fus::server::e_hasShutdownTimer;
    uv_stop(uv_default_loop());
}

// =================================================================================

static inline void output_result(bool result, const ST::string& success, const ST::string& fail)
{
    fus::console& c = fus::console::get();
    unsigned int resultsz = std::max(success.size(), fail.size());
    unsigned int column = std::min(70U, std::get<0>(c.size()) - resultsz);

    // And you thought the ansi was limited to console.cpp? For shame...
    c << "\x1B[" << column << "G" << fus::console::weight_bold;
    if (result)
        c << fus::console::foreground_green << success << fus::console::endl;
    else
        c << fus::console::foreground_red << fail << fus::console::endl;
}

void fus::server::daemon_ctl_noresult(const ST::string& action, const ST::string& daemon,
                                      daemon_ctl_noresult_f proc, const ST::string& success)
{
    FUS_ASSERTD(proc);

    console::get() << fus::console::weight_bold << action << " fus::" << daemon << console::flush;
    proc();
    output_result(true, success, ST::null);
}

bool fus::server::daemon_ctl_result(const ST::string& action, const ST::string& daemon,
                                    daemon_ctl_result_f proc, const ST::string& success,
                                    const ST::string& fail)
{
    FUS_ASSERTD(proc);

    console::get() << fus::console::weight_bold << action << " fus::" << daemon << console::flush;
    bool result = proc();
    output_result(result, success, fail);
    return result;
}

// =================================================================================

bool fus::server::config2addr(const ST::string& section, sockaddr_storage* addr)
{
    const ST::string& addrstr = m_config.get<const ST::string&>(section, ST_LITERAL("addr"));
    unsigned int port = m_config.get<unsigned int>(section, ST_LITERAL("port"));
    if (addrstr.empty()) {
        const char* bindaddr = m_config.get<const char*>("lobby", "bindaddr");
        return str2addr(bindaddr, port, addr);
    } else {
        return str2addr(addrstr.c_str(), (uint16_t)port, addr);
    }
}

void fus::server::fill_common_connection_header(void* packet)
{
    auto header = (fus::protocol::common_connection_header*)packet;
    header->set_msgsz(sizeof(fus::protocol::common_connection_header) - 4); // does not include the buf field
    header->set_buildId(m_config.get<unsigned int>("client", "buildId"));
    header->set_buildType(m_config.get<unsigned int>("client", "buildType"));
    header->set_branchId(m_config.get<unsigned int>("client", "branchId"));
    header->get_product()->from_string(m_config.get<const char*>("client", "product"));
    header->set_bufsz(0);
}
