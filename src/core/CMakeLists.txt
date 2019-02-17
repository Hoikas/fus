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

include_directories(${STRING_THEORY_INCLUDE_DIRS})
if (NOT WIN32)
    include_directories(${LibUUID_INCLUDE_DIR})
endif()

set(FUS_CORE_HEADERS
    build_info.h
    config_parser.h
    endian.h
    errors.h
    list.h
    uuid.h
    "${PROJECT_BINARY_DIR}/include/fus_config.h"
)

set(FUS_CORE_SOURCES
    build_info.cpp
    config_parser.cpp
    errors.cpp
    uuid.cpp
)

add_library(fus_core STATIC ${FUS_CORE_HEADERS} ${FUS_CORE_SOURCES})
target_link_libraries(fus_core buildinfoobj)
target_link_libraries(fus_core ${STRING_THEORY_LIBRARIES})
if(WIN32)
    target_link_libraries(fus_core Rpcrt4)
else()
    target_link_libraries(fus_core LibUUID::LibUUID)
endif()

source_group("Header Files" FILES ${FUS_CORE_HEADERS})
source_group("Source Files" FILES ${FUS_CORE_SOURCES})
