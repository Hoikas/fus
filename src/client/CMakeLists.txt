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

include_directories(${LIBUV_INCLUDE_DIRS})
include_directories(${STRING_THEORY_INCLUDE_DIRS})
include_directories("../")

set(FUS_CLIENT_HEADERS
    admin_client.h
    db_client.h
    client_base.h
)

set(FUS_CLIENT_SOURCES
    admin_client.cpp
    db_client.cpp
    client_base.cpp
)

add_library(fus_client STATIC ${FUS_CLIENT_HEADERS} ${FUS_CLIENT_SOURCES})
target_link_libraries(fus_client ${LIBUV_LIBRARIES})
target_link_libraries(fus_client ${STRING_THEORY_LIBRARIES})
target_link_libraries(fus_client fus_core)
target_link_libraries(fus_client fus_io)
target_link_libraries(fus_client fus_protocol)

source_group("Header Files" FILES ${FUS_CLIENT_HEADERS})
source_group("Source Files" FILES ${FUS_CLIENT_SOURCES})
