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

#include "errors.h"

#ifdef _MSC_VER
#   include <crtdbg.h>
#endif
#include <string_theory/st_stringstream.h>

// =================================================================================

static void _default_assert_handler(const char* cond, const char* file, int line, const char* msg)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    ST::string_stream ss;
    ss << "Condition: " << cond;
    if (msg)
        ss << "\r\nMessage: " << msg;

    /// FIXME: Should probably use _CrtDbgReportW
    int dbg = _CrtDbgReport(_CRT_ASSERT, file, line, nullptr, ss.to_string().c_str());
    if (dbg == 1)
        __debugbreak();
#else
    /// FIXME: this sucks
    abort();
#endif
}

// =================================================================================

static fus::assert::handler_t s_assertHandler = _default_assert_handler;

// =================================================================================

void fus::assert::handle(const char* cond, const char* file, int line, const char* msg)
{
    if (s_assertHandler)
        s_assertHandler(cond, file, line, msg);
}

// =================================================================================

void fus::assert::reset_handler()
{
    s_assertHandler = _default_assert_handler;
}

void fus::assert::set_handler(fus::assert::handler_t handler)
{
    s_assertHandler = handler;
}
