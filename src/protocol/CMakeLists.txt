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
include_directories("../io")

set(FUS_PROTOCOL_HEADERS
    common.h
)

set(FUS_PROTOCOL_INLINES
    protocol_fields_begin.inl
    protocol_fields_end.inl
    protocol_objects_begin.inl
    protocol_objects_end.inl
    protocol_structs_begin.inl
    protocol_structs_end.inl
)

set(FUS_PROTOCOL_PROTOCOLS
    common.inl
)

set(FUS_PROTOCOL_SOURCES
    protocol.cpp
)

add_library(fus_protocol STATIC ${FUS_PROTOCOL_HEADERS} ${FUS_PROTOCOL_INLINES}
                                ${FUS_PROTOCOL_PROTOCOLS} ${FUS_PROTOCOL_SOURCES})
target_link_libraries(fus_protocol fus_io)

source_group("Header Files" FILES ${FUS_PROTOCOL_HEADERS})
source_group("Inline Files" FILES ${FUS_PROTOCOL_INLINES})
source_group("Protocol Files" FILES ${FUS_PROTOCOL_PROTOCOLS})
source_group("Source Files" FILES ${FUS_PROTOCOL_SOURCES})
