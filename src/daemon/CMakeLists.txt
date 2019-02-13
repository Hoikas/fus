#    This file is part of fus.
#
#    fus is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    fus is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with fus.  If not, see <https://www.gnu.org/licenses/>.

include_directories(${GFLAGS_INCLUDE_DIRS})
include_directories(${LIBUV_INCLUDE_DIRS})
include_directories(${STRING_THEORY_INCLUDE_DIRS})
include_directories("../")

set(FUS_ADMIN_DAEMON
    adminsrv/admin.h
    adminsrv/admin_daemon.cpp
    adminsrv/admin_server.cpp
    adminsrv/admin_private.h
)

set(FUS_AUTH_DAEMON
    authsrv/auth.h
    authsrv/auth_daemon.cpp
    authsrv/auth_server.cpp
    authsrv/auth_private.h
)

set(FUS_DAEMON_HEADERS
    daemon_base.h
    daemon_config.h
    server.h
)

set(FUS_DAEMON_SOURCES
    daemon_base.cpp
    main.cpp
    server.cpp
    server_console.cpp
)

add_executable(fus_daemon ${FUS_ADMIN_DAEMON} ${FUS_AUTH_DAEMON} ${FUS_DAEMON_HEADERS} ${FUS_DAEMON_SOURCES})
target_link_libraries(fus_daemon ${GFLAGS_LIBRARIES})
target_link_libraries(fus_daemon ${LIBUV_LIBRARIES})
target_link_libraries(fus_daemon ${STRING_THEORY_LIBRARIES})
target_link_libraries(fus_daemon fus_client)
target_link_libraries(fus_daemon fus_core)
target_link_libraries(fus_daemon fus_io)
target_link_libraries(fus_daemon fus_protocol)

source_group("Admin Daemon" FILES ${FUS_ADMIN_DAEMON})
source_group("Auth Daemon" FILES ${FUS_AUTH_DAEMON})
source_group("Header Files" FILES ${FUS_DAEMON_HEADERS})
source_group("Source Files" FILES ${FUS_DAEMON_SOURCES})
