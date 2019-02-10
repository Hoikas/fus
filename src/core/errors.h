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

#ifndef __FUS_ERROR_H
#define __FUS_ERROR_H

#include <functional>
#include <string_theory/format>

#ifndef NDEBUG
#   define FUS_ASSERTD(cond) if ((cond) == 0) { fus::assert::handle(#cond, __FILE__, __LINE__, nullptr); }
#   define FUS_ASSERTD_MSG(cond, msg, ...) if ((cond) == 0) { fus::assert::_handle(#cond, __FILE__, __LINE__, msg, __VA_ARGS__); }
#else
#   define FUS_ASSERTD(cond) (cond);
#   define FUS_ASSERTD_MSG(cond, msg, ...) (cond);
#endif
#define FUS_ASSERTR(cond) if ((cond) == 0) { fus::assert::handle(#cond, __FILE__, __LINE__, nullptr); }
#define FUS_ASSERTR_MSG(cond, msg, ...) if ((cond) == 0) { fus::assert::_handle(#cond, __FILE__, __LINE__, msg, __VA_ARGS__); }

namespace fus
{
    class assert
    {
    public:
        typedef std::function<void(const char*, const char*, int, const char*)> handler_t;

        static void handle(const char* cond, const char* file, int line, const char* msg);
        template<typename... args>
        static inline void _handle(const char* cond, const char* file, int line, const char* fmt, args... _Args)
        {
            ST::string msg = ST::format(fmt, _Args...);
            handle(cond, file, line, msg.c_str());
        }

        static void reset_handler();
        static void set_handler(handler_t handler);
    };
};

#endif
