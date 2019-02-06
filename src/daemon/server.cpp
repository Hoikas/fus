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

#include "admin.h"
#include "auth.h"
#include "client/admin_client.h"
#include "core/errors.h"
#include "fus_config.h"
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

// =================================================================================

fus::server* fus::server::m_instance = nullptr;

enum
{
    e_lobbyReady = (1<<0),
    e_running = (1<<1),
};

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
}

fus::server::~server()
{
    m_instance = nullptr;
    if (m_admin) {
        admin_client_free(m_admin);
        m_admin = nullptr;
    }
    shutdown_daemons();
    console::get().end(); // idempotent
    /// fixme: fus shutdown not properly implemented
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

    fus::protocol::connection_header* header = (fus::protocol::connection_header*)msg;
    switch (header->get_connType()) {
    case fus::protocol::e_protocolCli2Admin:
        log.write_debug("[{}] Incoming admin connection", fus::tcp_stream_peeraddr(client));
        fus::admin_daemon_accept((fus::admin_server_t*)client, msg);
        break;
    case fus::protocol::e_protocolCli2Auth:
        log.write_debug("[{}] Incoming auth connection", fus::tcp_stream_peeraddr(client));
        fus::auth_daemon_accept((fus::auth_server_t*)client, msg);
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
        fus::tcp_stream_read_msg<fus::protocol::connection_header>(client, _on_header_read);
    } else {
        uv_close((uv_handle_t*)client, (uv_close_cb)fus::tcp_stream_free);
    }
}

bool fus::server::start_lobby()
{
    FUS_ASSERTD(!(m_flags & e_lobbyReady));
    FUS_ASSERTD(!(m_flags & e_running));
    init_daemons();

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

void fus::server::init_daemons()
{
    if (FLAGS_run_admin)
        admin_daemon_init();
    if (FLAGS_run_auth)
        auth_daemon_init();
}

void fus::server::shutdown_daemons()
{
    if (admin_daemon_running())
        admin_daemon_free();
    if (auth_daemon_running())
        auth_daemon_free();
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

void fus::server::fill_connection_header(void* packet)
{
    auto header = (fus::protocol::connection_header*)packet;
    header->set_msgsz(sizeof(fus::protocol::connection_header) - 4); // does not include the buf field
    header->set_buildId(m_config.get<unsigned int>("client", "buildId"));
    header->set_buildType(m_config.get<unsigned int>("client", "buildType"));
    header->set_branchId(m_config.get<unsigned int>("client", "branchId"));
    header->get_product()->from_string(m_config.get<const char*>("client", "product"));
    header->set_bufsz(0);
}
