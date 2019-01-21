#    This file is part of fus.
#
#    Foobar is free software: you can redistribute it and/or modify
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

set(FUS_CORE_HEADERS
    config_parser.h
    errors.h
    list.h
)

set(FUS_CORE_SOURCES
    config_parser.cpp
    errors.cpp
)

add_library(fus_core STATIC ${FUS_CORE_HEADERS} ${FUS_CORE_SOURCES})
target_link_libraries(fus_core ${STRING_THEORY_LIBRARIES})

source_group("Header Files" FILES ${FUS_CORE_HEADERS})
source_group("Source Files" FILES ${FUS_CORE_SOURCES})
