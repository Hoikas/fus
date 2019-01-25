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

#include "auth.h"
#include "core/errors.h"
#include "fus_config.h"
#include "io/net_struct.h"
#include "io/tcp_stream.h"
#include "protocol/common.h"
#include "server.h"

fus::server* fus::server::m_instance = nullptr;

enum
{
    e_lobbyReady = (1<<0),
    e_running = (1<<1),
};

enum
{
    // Externally facing services
    e_protocolCli2Auth = 0x0A,
    e_protocolCli2Game = 0x0B,
    e_protocolCli2File = 0x10,
    e_protocolCli2Gate = 0x16,

    // Internal services
    e_protocolSrv2Master = 0x80,
    e_protocolSrv2Database = 0x81,
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

/// FIXME: remove base types when more clients are available
constexpr size_t k_clientMemsz = max_sizeof<fus::auth_client_t,
                                            fus::crypt_stream_t,
                                            fus::tcp_stream_t>::value;

// =================================================================================

fus::server::server(const std::filesystem::path& config_path)
    : m_config(fus::daemon_config), m_flags()
{
    m_instance = this;
    m_config.read(config_path);
}

fus::server::~server()
{
    m_instance = nullptr;
}

// =================================================================================

static void _on_header_read(fus::tcp_stream_t* client, ssize_t error, void* msg)
{
    if (error < 0) {
        std::cerr << "Read error " << uv_strerror(error) << std::endl;
        uv_close((uv_handle_t*)client, nullptr);
        return;
    }

    fus::protocol::connection_header* header = (fus::protocol::connection_header*)msg;
    /// TODO: validate connection properties
    switch (header->get_connType()) {
    case e_protocolCli2Auth:
        if (fus::auth_daemon_running()) {
            fus::auth_daemon_accept_client((fus::auth_client_t*)client);
        } else {
            // ...
        }
        break;
    }
}

static void _on_client_connect(uv_stream_t* lobby, int status)
{
    if (status < 0) {
        std::cerr << "New connection error " << uv_strerror(status) << std::endl;
        return;
    }

    // We absolutely must allocate enough space for the largest client type. It would be nice
    // if we could realloc() when we knew what the connection type is, but that could result in
    // the pointer address changing. That's a generally a bad thing for non-POD, like us.
    fus::tcp_stream_t* client = (fus::tcp_stream_t*)malloc(k_clientMemsz);
    fus::tcp_stream_init(client, uv_default_loop());
    if (uv_accept(lobby, (uv_stream_t*)client) == 0) {
        fus::tcp_stream_read_msg<fus::protocol::connection_header>(client, _on_header_read);
    } else {
        uv_close((uv_handle_t*)client, nullptr);
    }
}

bool fus::server::start_lobby()
{
    FUS_ASSERTD(!(m_flags & e_lobbyReady));
    FUS_ASSERTD(!(m_flags & e_running));

    const char* bindaddr = m_config.get<const char*>("lobby", "bindaddr");
    int port = m_config.get<int>("lobby", "port");

    uv_loop_t* loop = uv_default_loop();
    uv_tcp_init(loop, &m_lobby);
    struct sockaddr_in addr;
    FUS_ASSERTD(uv_ip4_addr(bindaddr, port, &addr) == 0);
    if (uv_tcp_bind(&m_lobby, (const struct sockaddr*)&addr, 0) < 0)
        return false;
    if (uv_listen((uv_stream_t*)&m_lobby, 128, _on_client_connect) < 0)
        return false;
    m_flags |= e_lobbyReady;
    return true;
}

// =================================================================================

void fus::server::run_forever()
{
    FUS_ASSERTD((m_flags & e_lobbyReady));
    FUS_ASSERTD(!(m_flags & e_running));

    m_flags |= e_running;
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    m_flags &= ~e_running;
}

void fus::server::run_once()
{
    FUS_ASSERTD((m_flags & e_lobbyReady));
    FUS_ASSERTD(!(m_flags & e_running));

    m_flags |= e_running;
    uv_run(uv_default_loop(), UV_RUN_ONCE);
    m_flags &= ~e_running;
}
