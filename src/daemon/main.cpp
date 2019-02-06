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

#include "core/build_info.h"
#include <gflags/gflags.h>
#include "io/console.h"
#include "io/io.h"
#include "server.h"

// =================================================================================

DEFINE_string(config_path, "fus.ini", "Path to fus configuration file");
DEFINE_string(generate_client_ini, "", "Generates a server.ini file for plClient");
DEFINE_bool(generate_keys, false, "Generate a new set of encryption keys");
DEFINE_bool(run_lobby, true, "Launch the server lobby");
DEFINE_bool(save_config, false, "Saves the server configuration file");
DEFINE_bool(use_console, true, "Use the interactive server console");

// =================================================================================

int main(int argc, char* argv[])
{
    gflags::SetVersionString(fus::build_version());
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    fus::console& console = fus::console::init(uv_default_loop());
    console << fus::console::foreground_yellow << fus::console::weight_bold << fus::ro::dah() << "\n\n" << fus::console::flush;

    fus::io_init();
    {
        fus::server server(FLAGS_config_path);

        // Do anything that might change the server's configuration here and optionally save the new
        // configuration file at the end of the process.
        if (FLAGS_generate_keys)
            server.generate_daemon_keys();
        if (!FLAGS_generate_client_ini.empty())
            server.generate_client_ini(FLAGS_generate_client_ini);
        if (FLAGS_save_config)
            server.config().write(FLAGS_config_path);

        // Init the daemons
        if (FLAGS_use_console)
            server.start_console();

        // Run the lobby server to accept connections and pump the loop forever
        if (FLAGS_run_lobby)
            server.start_lobby();

        // Run the thingy...
        server.run_forever();
    }
    fus::io_close();
    return 0;
}
