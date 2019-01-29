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

add_custom_target(buildinfo ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_buildinfo.cpp)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.cpp
                          ${CMAKE_CURRENT_BINARY_DIR}/_buildinfo.cpp
                   COMMAND ${CMAKE_COMMAND} -P
                           ${CMAKE_MODULE_PATH}/buildinfo.cmake)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/buildinfo.cpp PROPERTIES GENERATED TRUE)
add_library(buildinfoobj STATIC ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.cpp)
add_dependencies(buildinfoobj buildinfo)
