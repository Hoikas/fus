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

cmake_minimum_required(VERSION 3.2)
project(fus)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${fus_BINARY_DIR}/bin)

# Required third party libraries
find_package(gflags REQUIRED)
find_package(libuv REQUIRED)
if (NOT WIN32)
    find_package(LibUUID REQUIRED)
endif()
include(OpenSSL) # some extra logic in there
find_package(string_theory REQUIRED)

# Optional Stuff
option(FUS_DB_SQLITE3 "Use SQLite3 Database Engine" ON)
if(FUS_DB_SQLITE3)
    find_package(SQLite3 REQUIRED)
    set(FUS_HAVE_SQLITE 1)
else()
    find_package(SQLite3 QUIET)
    set(FUS_HAVE_SQLITE 0)
endif()

include_directories("${PROJECT_BINARY_DIR}/include")
configure_file("${PROJECT_SOURCE_DIR}/src/fus_config.h.in" "${PROJECT_BINARY_DIR}/include/fus_config.h")

# Might as well request C++17 so the mice can play.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent Win32 junk from leaking in
if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

# Disable MSVC++ warning C4200
# "nonstandard extension used: zero-sized array in struct/union"
if(MSVC)
    add_compile_options(/wd4200)
endif()

# Allow the libs and exes to fondle themselves
add_subdirectory(src)
