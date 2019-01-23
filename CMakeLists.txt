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

# Required third party libraries
find_package(gflags REQUIRED)
find_package(LibUV REQUIRED)
include(OpenSSL) # some extra logic in there
find_package(string_theory REQUIRED)

# Might as well request C++17 so the mice can play.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow the libs and exes to fondle themselves
add_subdirectory(src)
