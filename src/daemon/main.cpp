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
#include <uv.h>

#include "common.h"
#include "fus_config.h"
#include "net_struct.h"
#include "tcp_stream.h"

// =================================================================================

DEFINE_string(config_path, "fus.ini", "Path to fus configuration file");

// =================================================================================

static void on_header_read(fus::tcp_stream_t* client, ssize_t error, void* msg)
{
    if (error < 0) {
        std::cerr << "Read error " << uv_strerror(error) << std::endl;
        uv_close((uv_handle_t*)client, nullptr);
        uv_stop(uv_default_loop());
        return;
    }

    // dump to console
    fus::net_msg_print(fus::protocol::connection_header::net_struct, msg, std::cout);
    uv_close((uv_handle_t*)client, nullptr);
    uv_stop(uv_default_loop());
}

static void on_new_connection(uv_stream_t* server, int status)
{
    if (status < 0) {
        std::cerr << "New connection error " << uv_strerror(status) << std::endl;
        uv_stop(uv_default_loop());
        return;
    }

    // using malloc() because we will eventually want to realloc into the final client type
    fus::tcp_stream_t* client = (fus::tcp_stream_t*)malloc(sizeof(fus::tcp_stream_t));
    fus::tcp_stream_init(client, uv_default_loop());
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        fus::tcp_stream_read_msg(client, fus::protocol::connection_header::net_struct, on_header_read);
    } else {
        uv_close((uv_handle_t*)client, nullptr);
        uv_stop(uv_default_loop());
    }
}

// =================================================================================

int main(int argc, char* argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    // TEST: display common stuff
    net_struct_print(fus::protocol::connection_header::net_struct, std::cout);
    net_struct_print(fus::protocol::msg_std_header::net_struct, std::cout);
    net_struct_print(fus::protocol::msg_size_header::net_struct, std::cout);

    // TEST: Load a config file
    fus::config_parser config(fus::daemon_config);
    config.read(FLAGS_config_path);
    const char* bindaddr = config.get<const char*>("lobby", "bindaddr");
    int port = config.get<int>("lobby", "port");

    // Try to receive a connection header from a client...
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_t server; // todo: don't use this directly
    uv_tcp_init(loop, &server);
    struct sockaddr_in addr;
    uv_ip4_addr(bindaddr, port, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    if (r) {
        std::cerr << "Listen error " << uv_strerror(r) << std::endl;
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}
