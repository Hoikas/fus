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

#include "client/admin_client.h"
#include <fstream>
#include "io/console.h"
#include "io/io.h"
#include "protocol/admin.h"
#include "server.h"
#include <string_theory/iostream>
#include <string_theory/st_format.h>
#include <vector>

// =================================================================================

static void admin_wallBCast(fus::admin_client_t* client, const ST::string& sender, const ST::string& text)
{
    fus::console& c = fus::console::get();
    c << fus::console::weight_bold << fus::console::foreground_cyan << "From " << sender << ": "
      << fus::console::weight_normal << fus::console::foreground_white << text << fus::console::endl;
}

// =================================================================================

static void admin_connected(fus::admin_client_t* client, ssize_t status)
{
    fus::console& c = fus::console::get();
    if (status < 0) {
        c << fus::console::weight_bold << fus::console::foreground_red
          << "Error: Connection to AdminSrv failed (" << uv_strerror(status) << ") - retrying in 5s"
          << fus::console::endl;
        fus::client_reconnect((fus::client_t*)client, 5000);
    } else {
        c << fus::console::foreground_green << "Connected to AdminSrv "
          << fus::tcp_stream_peeraddr((fus::tcp_stream_t*)client) << fus::console::endl;
    }
}

void fus::server::admin_disconnected(fus::admin_client_t* client)
{
    fus::server* server = (fus::server*)uv_handle_get_data((uv_handle_t*)client);
    fus::console& c = fus::console::get();

    c << fus::console::weight_bold << fus::console::foreground_red << "Disconnected from AdminSrv";
    if (!(server->m_flags & fus::server::e_shuttingDown)) {
        c << " - reconnecting in 5s";
        fus::client_reconnect((fus::client_t*)client, 5000);
    }
    c << fus::console::endl;
}

void fus::server::admin_init()
{
    m_admin = (admin_client_t*)malloc(sizeof(admin_client_t));
    admin_client_init(m_admin, uv_default_loop());

    uv_handle_set_data((uv_handle_t*)m_admin, this);
    tcp_stream_close_cb((tcp_stream_t*)m_admin, (uv_close_cb)admin_disconnected);
    admin_client_wall_handler(m_admin, admin_wallBCast);

    unsigned int g = m_config.get<unsigned int>("crypt", "admin_g");
    const ST::string& n = m_config.get<const ST::string&>("crypt", "admin_n");
    const ST::string& x = m_config.get<const ST::string&>("crypt", "admin_x");

    sockaddr_storage addr;
    config2addr(ST_LITERAL("admin"), &addr);

    void* header = alloca(admin_client_header_size());
    fill_connection_header(header);
    admin_client_connect(m_admin, (sockaddr*)&addr, header, admin_client_header_size(), g, n, x, (client_connect_cb)admin_connected);
}

bool fus::server::admin_check(console& console) const
{
    if (!tcp_stream_connected((const tcp_stream_t*)m_admin)) {
        console << console::foreground_red << console::weight_bold << "Error: AdminSrv not available" << console::endl;
        return false;
    }
    return true;
}

static void admin_acctCreated(void*, fus::admin_client_t*, uint32_t, fus::net_error result, ssize_t, const void*)
{
    fus::console& c = fus::console::get();
    if (result == fus::net_error::e_success) {
        c << fus::console::foreground_green << fus::console::weight_bold << "Account created!" << fus::console::endl;
    } else {
        c << fus::console::foreground_red << fus::console::weight_bold << "Error creating account: "
          << fus::net_error_string(result) << fus::console::endl;
    }
}

bool fus::server::admin_acctCreate(console& console, const ST::string& line)
{
    if (!admin_check(console))
        return true;

    std::vector<ST::string> args = line.split(' ');
    if (args.size() < 2)
        return false;

    /// FIXME: acct flags...
    admin_client_create_account(m_admin, args[0], args[1], 0, (client_trans_cb)admin_acctCreated);
    return true;
}


static void admin_pong(void*, fus::admin_client_t* client, uint32_t, fus::net_error result, ssize_t nread, const fus::protocol::admin_pingReply* pong)
{
    if (result != fus::net_error::e_success) {
        fus::console::get() << fus::console::foreground_red << fus::console::weight_bold << "Error: Ping failed" << fus::console::endl;
        return;
    }
    uint32_t nowTimeMs = (uint32_t)uv_now(uv_default_loop());
    uint32_t pingpong = nowTimeMs - pong->get_pingTime();

    fus::console::get() << fus::console::foreground_cyan << fus::console::weight_bold << "PONG ("
                        << pingpong << "ms)!" << fus::console::endl;
}

bool fus::server::admin_ping(console& console, const ST::string&)
{
    if (!admin_check(console))
        return true;

    // I know, I know...
    uint32_t pingTimeMs = (uint32_t)uv_now(uv_default_loop());
    admin_client_ping(m_admin, pingTimeMs, (client_trans_cb)admin_pong);
    return true;
}

bool fus::server::admin_wall(console& console, const ST::string& msg)
{
    if (msg.empty())
        return false;
    if (admin_check(console))
        admin_client_wall(m_admin, msg);
    return true;
}

// =================================================================================

bool fus::server::daemon_ctl(fus::console& console, const ST::string& line)
{
    std::vector args = line.split(' ');
    if (args.size() != 2)
        return false;

    auto it = m_daemonCtl.find(args[1]);
    if (it == m_daemonCtl.end()) {
        console << console::weight_bold << console::foreground_red << "Error: Unknown server type '"
                << args[1] << "'" << console::endl;
        return true;
    }

    ST::string action = args[0].to_lower();
    if (action == ST_LITERAL("start")) {
        if (it->second.running()) {
            console << console::weight_bold << console::foreground_yellow << "fus::" << it->first
                    << " already running!" << console::endl;
        } else {
            daemon_ctl_result("Starting", it->first, it->second.init, "[  OK  ]", "[FAILED]");
        }
    } else if (action == ST_LITERAL("status")) {
        daemon_ctl_result("Status of", it->first, it->second.running, "[RUNNING]", "[STOPPED]");
    } else {
        console << console::weight_bold << console::foreground_red << "Unknown option '"
                << action << "'" << console::endl;
    }
    return true;
}

bool fus::server::generate_keys(fus::console& console, const ST::string& args)
{
    if (args.empty()) {
        generate_daemon_keys();
    } else {
        std::vector<ST::string> srvs = args.split(' ');
        for (const ST::string& srv : srvs)
            generate_daemon_keys(srv);
    }
    return true;
}

bool fus::server::quit(fus::console& console, const ST::string&)
{
    console << console::weight_bold << console::foreground_cyan << "Shutting down local fus daemon..." << console::endl;
    shutdown();
    return true;
}

bool fus::server::save_config(fus::console& console, const ST::string& args)
{
    bool srv_config = true;
    std::filesystem::path path = "fus.ini";

    if (!args.empty()) {
        std::vector<ST::string> params = args.split(' ', 1);
        srv_config = params[0].to_lower() == ST_LITERAL("server");
        if (args.size() > 1)
            path = params[1].to_path();
    }

    if (srv_config)
        m_config.write(path);
    else
        generate_client_ini(path);
    console << "Saved ";
    if (srv_config)
        console << "server ";
    else
        console << "client ";
    console << "configuration to '" << path.c_str() << "'" << console::endl;
    return true;
}

// =================================================================================

void fus::server::start_console()
{
    console& console = console::get();

    // Add all console commands.
    console.add_command("addacct", "addacct [name] [password] [flags]", "Creates a new account for logging into the game",
                        std::bind(&fus::server::admin_acctCreate, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("config", "config [server|client] [output]", "Generates fus or plClient configuration",
                        std::bind(&fus::server::save_config, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("daemonctl", "daemonctl [start|status] [server]", "Observe or manipulate the status of fus daemons",
                        std::bind(&fus::server::daemon_ctl, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("keygen", "keygen [server(s)]", "Generates encryption keys for the requested servers",
                        std::bind(&fus::server::generate_keys, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("ping", "ping", "Pings the admin daemon",
                        std::bind(&fus::server::admin_ping, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("quit", "quit", "Shuts down the local fus daemon",
                        std::bind(&fus::server::quit, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("wall", "wall [msg]", "Sends a message to all server consoles and players in the cavern",
                        std::bind(&fus::server::admin_wall, this, std::placeholders::_1, std::placeholders::_2));

    // Many commands require deep integration with the server, so we will naughtily connect to ourselves
    admin_init();

    console.begin();
    console << console::endl;
}

// =================================================================================

void fus::server::generate_daemon_keys()
{
    for (auto& it : m_daemonCtl)
        generate_daemon_keys(it.first);
}

void fus::server::generate_daemon_keys(const ST::string& srv)
{
    console& c = console::get();
    auto it = m_daemonCtl.find(srv);
    if (it == m_daemonCtl.end()) {
        c << console::weight_bold << console::foreground_red << "Error: Unknown server type '"
          << srv << "'" << console::endl;
        return;
    }

    c << console::weight_bold << console::foreground_yellow << "Generating keys for fus::"
      << srv << console::endl;

    ST::string section = ST_LITERAL("crypt");
    unsigned int g_value = m_config.get<unsigned int>(section, ST::format("{}_g", srv));
    auto keys = fus::io_generate_keys(g_value);
    m_config.set<const ST::string&>(section, ST::format("{}_k", srv), std::get<0>(keys));
    m_config.set<const ST::string&>(section, ST::format("{}_n", srv), std::get<1>(keys));
    m_config.set<const ST::string&>(section, ST::format("{}_x", srv), std::get<2>(keys));
}

// =================================================================================

static void generate_client_keys(const fus::config_parser& config, std::ostream& stream, const ST::string& srv)
{
    ST::string srv_lower = srv.to_lower();
    unsigned int g_val = config.get<unsigned int>(ST_LITERAL("crypt"), ST::format("{}_g", srv_lower));
    const char* n_key = config.get<const char*>(ST_LITERAL("crypt"), ST::format("{}_n", srv_lower));
    const char* x_key = config.get<const char*>(ST_LITERAL("crypt"), ST::format("{}_x", srv_lower));

    stream << "Server." << srv << ".G " << g_val << std::endl;
    stream << "Server." << srv << ".N \"" << n_key << "\"" << std::endl;
    stream << "Server." << srv << ".X \"" << x_key << "\"" << std::endl;
}

void fus::server::generate_client_ini(const std::filesystem::path& path) const
{
    std::ofstream stream;
    stream.open(path, std::ios_base::out | std::ios_base::trunc);

    // Encryption keys
    generate_client_keys(m_config, stream, ST_LITERAL("Auth"));
    generate_client_keys(m_config, stream, ST_LITERAL("Gate"));
    generate_client_keys(m_config, stream, ST_LITERAL("Game"));

    // Shard addresses
    stream << std::endl;
    const ST::string& extaddr = m_config.get<const ST::string&>("lobby", "extaddr");
    const ST::string& bindaddr = m_config.get<const ST::string&>("lobby", "bindaddr");
    stream << "Server.Auth.Host \"" << (extaddr.empty() ? bindaddr : extaddr) << "\"" << std::endl;
}
