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

#include <gflags/gflags.h>
#include <iostream>

#include "server.h"

// =================================================================================

DEFINE_string(config_path, "fus.ini", "Path to fus configuration file");
DEFINE_bool(run_lobby, true, "Launch the server lobby");
DEFINE_bool(save_config, false, "Saves the server configuration file");

// =================================================================================

int main(int argc, char* argv[])
{
    /// fixme: set program name and version
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    fus::server server(FLAGS_config_path);
    if (FLAGS_run_lobby) {
        if (server.start_lobby())
            server.run_forever();
    }
    if (FLAGS_save_config) {
        server.config().write(FLAGS_config_path);
    }

    return 0;
}
