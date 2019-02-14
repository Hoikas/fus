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
if(FUS_HAVE_SQLITE)
    include_directories(${SQLITE3_INCLUDE_DIRS})
endif()
include_directories(${STRING_THEORY_INCLUDE_DIRS})
include_directories("../")

set(FUS_DB_HEADERS
    database.h
)

set(FUS_DB_SOURCES
    database.cpp
)

if (FUS_HAVE_SQLITE)
    set(FUS_DB_SQLITE_SOURCES
        db_sqlite3.h
        sqlite3_acct.cpp
        sqlite3_init.cpp
        sqlite3_query.cpp
    )
else()
    set(FUS_DB_SQLITE_SOURCES)
endif()

add_library(fus_db STATIC ${FUS_DB_HEADERS} ${FUS_DB_SOURCES} ${FUS_DB_SQLITE_SOURCES})
target_link_libraries(fus_db ${LIBUV_LIBRARIES})
if(FUS_HAVE_SQLITE)
    target_link_libraries(fus_db ${SQLITE3_LIBRARIES})
endif()
target_link_libraries(fus_db ${STRING_THEORY_LIBRARIES})
target_link_libraries(fus_db fus_core)

source_group("Header Files" FILES ${FUS_DB_HEADERS})
source_group("Source Files" FILES ${FUS_DB_SOURCES})
source_group("SQLite3 DB" FILES ${FUS_DB_SQLITE_SOURCES})
