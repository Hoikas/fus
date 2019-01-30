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

#ifndef __FUS_LOG_FILE_H
#define __FUS_LOG_FILE_H

#include <string_theory/st_format.h>
#include <uv.h>

namespace fus
{
    class log_file final
    {
    public:
        enum class level
        {
            e_debug,
            e_info,
            e_error,
        };

    private:
        uv_file m_file;
        uv_loop_t* m_loop;
        level m_level;
        int64_t m_offset;

    public:
        log_file() : m_file(-1), m_loop(), m_level(level::e_info), m_offset() { }
        log_file(const log_file&) = delete;
        log_file(log_file&&) = delete;

        static void set_directory(const ST::string&);

        int open(uv_loop_t*, const ST::string&);
        void close();

        void set_level(level);
        void set_level(const ST::string&);
        void write(const ST::string&);

        template<typename... _Args>
        void write(const char* fmt, _Args ...args)
        {
            write(ST::format(fmt, args...));
        }

        template<size_t _Sz>
        void write(const char(*msg)[_Sz])
        {
            write(ST::string::from_literal(msg, _Sz - 1));
        }

        template<size_t _Sz>
        void write_debug(const char(*msg)[_Sz])
        {
            if (m_level <= level::e_debug)
                write(ST::string::from_literal(msg, _Sz - 1));
        }

        template<size_t _Sz>
        void write_error(const char(*msg)[_Sz])
        {
            if (m_level <= level::e_error)
                write(ST::string::from_literal(msg, _Sz - 1));
        }

        template<size_t _Sz>
        void write_info(const char(*msg)[_Sz])
        {
            if (m_level <= level::e_info)
                write(ST::string::from_literal(msg, _Sz - 1));
        }

        template<typename... _Args>
        void write_debug(const char* fmt, _Args ...args)
        {
            if (m_level <= level::e_debug)
                write(ST::format(fmt, args...));
        }

        template<typename... _Args>
        void write_error(const char* fmt, _Args ...args)
        {
            if (m_level <= level::e_error)
                write(ST::format(fmt, args...));
        }

        template<typename... _Args>
        void write_info(const char* fmt, _Args ...args)
        {
            if (m_level <= level::e_info)
                write(ST::format(fmt, args...));
        }
    };
};

#endif
