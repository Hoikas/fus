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
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${STRING_THEORY_INCLUDE_DIRS})
include_directories("../")

set(FUS_IO_HEADERS
    console.h
    crypt_stream.h
    hash.h
    io.h
    log_file.h
    net_struct.h
    net_error.h
    tcp_stream.h
)

set(FUS_IO_SOURCES
    console.cpp
    crypt_stream.cpp
    hash.cpp
    io.cpp
    log_file.cpp
    net_error.cpp
    net_struct.cpp
    tcp_stream.cpp
)

add_library(fus_io STATIC ${FUS_IO_HEADERS} ${FUS_IO_SOURCES})
target_link_libraries(fus_io ${LIBUV_LIBRARIES})
target_link_openssl_crypto(fus_io)
target_link_libraries(fus_io ${STRING_THEORY_LIBRARIES})
target_link_libraries(fus_io fus_core)

source_group("Header Files" FILES ${FUS_IO_HEADERS})
source_group("Source Files" FILES ${FUS_IO_SOURCES})
