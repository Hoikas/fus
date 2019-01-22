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

find_package(OpenSSL REQUIRED)

function(target_link_openssl_crypto target_name)
    target_link_libraries(${target_name} OpenSSL::Crypto)
    if(WIN32)
        target_link_libraries(${target_name} CRYPT32)
    endif()
endfunction()
