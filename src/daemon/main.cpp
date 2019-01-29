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
#include <gflags/gflags.h>
#include <iostream>
#include <string_theory/st_format.h>

#include "auth.h"
#include "core/build_info.h"
#include "io/io.h"
#include "server.h"

// =================================================================================

DEFINE_string(config_path, "fus.ini", "Path to fus configuration file");
DEFINE_string(generate_client_ini, "", "Generates a server.ini file for plClient");
DEFINE_bool(generate_keys, false, "Generate a new set of encryption keys");
DEFINE_bool(run_auth, true, "Launch the auth daemon");
DEFINE_bool(run_lobby, true, "Launch the server lobby");
DEFINE_bool(save_config, false, "Saves the server configuration file");

// =================================================================================

static void generate_client_keys(const fus::config_parser& config, std::ostream& stream, const ST::string& srv)
{
    ST::string srv_lower = srv.to_lower();
    unsigned int g_val = config.get<unsigned int>(ST_LITERAL("crypt"), ST::format("{}_g", srv_lower));
    const char* n_key = config.get<const char*>(ST_LITERAL("crypt"), ST::format("{}_n", srv_lower));
    const char* x_key = config.get<const char*>(ST_LITERAL("crypt"), ST::format("{}_x", srv_lower));

    stream << "Server." << srv.c_str() << ".G " << g_val << std::endl;
    stream << "Server." << srv.c_str() << ".N \"" << n_key << "\"" << std::endl;
    stream << "Server." << srv.c_str() << ".X \"" << x_key << "\"" << std::endl;
}

static void generate_client_ini(const fus::config_parser& config, const std::filesystem::path& path)
{
    std::ofstream stream;
    stream.open(path, std::ios_base::out | std::ios_base::trunc);

    // Encryption keys
    generate_client_keys(config, stream, ST_LITERAL("Auth"));
    generate_client_keys(config, stream, ST_LITERAL("Gate"));
    generate_client_keys(config, stream, ST_LITERAL("Game"));

    // Shard addresses
    /// todo
}

static void generate_daemon_keys(fus::config_parser& config, const ST::string& srv)
{
    ST::string section = ST_LITERAL("crypt");
    unsigned int g_value = config.get<unsigned int>(section, ST::format("{}_g", srv));
    auto keys = fus::io_generate_keys(g_value);
    config.set<const ST::string&>(section, ST::format("{}_k", srv), std::get<0>(keys));
    config.set<const ST::string&>(section, ST::format("{}_n", srv), std::get<1>(keys));
    config.set<const ST::string&>(section, ST::format("{}_x", srv), std::get<2>(keys));
}

static void generate_all_keys(fus::config_parser& config)
{
    generate_daemon_keys(config, ST_LITERAL("auth"));
    generate_daemon_keys(config, ST_LITERAL("game"));
    generate_daemon_keys(config, ST_LITERAL("gate"));
}

// =================================================================================

int main(int argc, char* argv[])
{
    fus::ro::dah(std::cout);

    gflags::SetVersionString(fus::build_version());
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    fus::io_init();
    fus::server server(FLAGS_config_path);

    // Do anything that might change the server's configuration here and optionally save the new
    // configuration file at the end of the process.
    if (FLAGS_generate_keys)
        generate_all_keys(server.config());
    if (!FLAGS_generate_client_ini.empty())
        generate_client_ini(server.config(), FLAGS_generate_client_ini);
    if (FLAGS_save_config)
        server.config().write(FLAGS_config_path);

    // Init the daemons
    if (FLAGS_run_auth)
        fus::auth_daemon_init();

    // Run the lobby server to accept connections and pump the loop forever
    if (FLAGS_run_lobby) {
        if (server.start_lobby())
            server.run_forever();
    }

    // Shutdown the daemons
    if (fus::auth_daemon_running())
        fus::auth_daemon_close();

    // Done
    fus::io_close();
    return 0;
}
