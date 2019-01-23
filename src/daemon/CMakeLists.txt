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
include_directories("../core")
include_directories("../io")
include_directories("../protocol")

set(FUS_DAEMON_HEADERS
    fus_config.h
    server.h
)

set(FUS_DAEMON_SOURCES
    main.cpp
    server.cpp
)

add_executable(fus_daemon ${FUS_DAEMON_HEADERS} ${FUS_DAEMON_SOURCES})
target_link_libraries(fus_daemon ${GFLAGS_LIBRARIES})
target_link_libraries(fus_daemon ${LIBUV_LIBRARIES})
target_link_libraries(fus_daemon ${STRING_THEORY_LIBRARIES})
target_link_libraries(fus_daemon fus_core)
target_link_libraries(fus_daemon fus_io)
target_link_libraries(fus_daemon fus_protocol)

source_group("Header Files" FILES ${FUS_DAEMON_HEADERS})
source_group("Source Files" FILES ${FUS_DAEMON_SOURCES})
