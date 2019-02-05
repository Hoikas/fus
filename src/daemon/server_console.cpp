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

#include <fstream>
#include "io/console.h"
#include "io/io.h"
#include "server.h"
#include <string_theory/iostream>
#include <string_theory/st_format.h>
#include <vector>

// =================================================================================

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
    fus::console& console = console::get();

    // Add all console commands.
    console.add_command("config", "config [server|client] [output]", "Generates fus or plClient configuration",
                        std::bind(&fus::server::save_config, this, std::placeholders::_1, std::placeholders::_2));
    console.add_command("keygen", "keygen [server(s)]", "Generates encryption keys for the requested servers",
                        std::bind(&fus::server::generate_keys, this, std::placeholders::_1, std::placeholders::_2));

    console.begin();
}

// =================================================================================

void fus::server::generate_daemon_keys()
{
    generate_daemon_keys(ST_LITERAL("admin"));
    generate_daemon_keys(ST_LITERAL("auth"));
    generate_daemon_keys(ST_LITERAL("game"));
    generate_daemon_keys(ST_LITERAL("gate"));
}

void fus::server::generate_daemon_keys(const ST::string& srv)
{
    console::get() << console::weight_bold << "Generating keys for '" << srv << "'..." << console::endl;

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
