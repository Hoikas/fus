/*   This file is part of fus.
 *
 *   fus is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   fus is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with fus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "build_info.h"
#include <iostream>
#include <string_theory/iostream>
#include <string_theory/st_stringstream.h>

// =================================================================================

namespace fus
{
    namespace buildinfo
    {
        extern const char* BUILD_HASH;
        extern const char* BUILD_TAG;
        extern const char* BUILD_BRANCH;
        extern const char* BUILD_DATE;
        extern const char* BUILD_TIME;
    };
};

// =================================================================================

const char* fus::build_date()
{
    return fus::buildinfo::BUILD_DATE;
}

const char* fus::build_hash()
{
    return fus::buildinfo::BUILD_HASH;
}

const char* fus::build_branch()
{
    return fus::buildinfo::BUILD_BRANCH;
}


const char* fus::build_tag()
{
    return fus::buildinfo::BUILD_TAG;
}

const char* fus::build_time()
{
    return fus::buildinfo::BUILD_TIME;
}

const char* fus::build_version()
{
    if (*fus::buildinfo::BUILD_TAG == 0)
        return fus::buildinfo::BUILD_HASH;
    else
        return fus::buildinfo::BUILD_TAG;
}

// =================================================================================

ST::string fus::ro::dah()
{
    ST::string_stream stream;
    stream << "fus - fast uru server";
    if (*fus::buildinfo::BUILD_TAG != 0)
        stream << " (" << fus::buildinfo::BUILD_TAG << ")";
    stream << " [" << fus::buildinfo::BUILD_HASH << " (" << fus::buildinfo::BUILD_BRANCH << ")] ";
    stream << fus::buildinfo::BUILD_DATE << " " << fus::buildinfo::BUILD_TIME << " ";
#if defined(__clang__)
    stream << "clang++ " << __clang_version__;
#elif defined(__GNUC__)
    stream << "g++ " << __VERSION__;
#elif defined(_MSC_VER)
    stream << "MSVC " << _MSC_VER;
#endif

    // Don't whine at me about this because I don't care right now.
    if (sizeof(void*) == 4)
        stream << " (x86)";
    else if (sizeof(void*) == 8)
        stream << " (x64)";
    return stream.to_string();
}

void fus::ro::dah(std::ostream& stream)
{
    stream << fus::ro::dah() << std::endl;
}
