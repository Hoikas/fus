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

#ifndef __FUS_CONSOLE_H
#define __FUS_CONSOLE_H

#include <deque>
#include <functional>
#include <map>
#include <string_theory/st_stringstream.h>
#include <tuple>
#include <uv.h>

namespace fus
{
    typedef std::function<bool(class console&, const ST::string&)> console_handler_t;
    class console_cmd
    {
    public:
        ST::string m_command;
        ST::string m_usage;
        ST::string m_desc;
        console_handler_t m_handler;
    };

    class console
    {
        enum class color
        {
            e_black, e_red, e_green, e_yellow, e_blue, e_magenta, e_cyan, e_white, e_default,
        };

        enum class weight
        {
            e_normal, e_bold
        };

        enum
        {
            e_processing = (1<<0),
            e_outputAnsiDirty = (1<<1),
            e_outputAnsiChanged = (1<<2),
            e_execCommand = (1<<3),
        };

        static console* m_instance;

        uv_tty_t m_stdin;
        uv_tty_t m_stdout;
        uv_write_t m_writeReq;

        ST::string m_prompt;
        ST::string_stream m_inputBuf;
        ST::string_stream m_outputBuf;
        char m_charBuf[32];
        uint32_t m_flags;
        uint64_t m_lastBell;

        std::deque<ST::string> m_history;
        std::deque<ST::string>::const_iterator m_historyIt;
        std::map<ST::string, console_cmd, ST::less_i> m_commands;

        unsigned int m_cursorPos;
        unsigned int m_promptSize;
        unsigned int m_inputCharacters;

        color m_outForeColor;
        color m_outBackColor;
        weight m_outWeight;

    protected:
        static size_t count_bytes(const char* buf, size_t bufsz, size_t charcount);
        static size_t count_characters(const char* buf, size_t characters=-1);

        static bool print_help(console& console, const ST::string& args);

        uint64_t now() const { return uv_now(uv_handle_get_loop((uv_handle_t*)&m_stdout)); }

        void ring_bell();
        void history_up(unsigned int value);
        void history_down(unsigned int value);
        void move_cursor_left(unsigned int value);
        void move_cursor_right(unsigned int value);
        void handle_backspace();
        void handle_delete();
        void fire_command(const ST::string& cmd, const ST::string& args);
        void handle_input_line();
        void handle_ctrl(const char* buf, size_t bufsz);
        void process_character(const char* buf, size_t bufsz);

        void delete_character();
        void insert_character(const char* buf, size_t bufsz);
        void flush_input_line();
        void set_input_line(const ST::string& value);

        static void allocate(uv_stream_t* stream, size_t suggestion, uv_buf_t* buf);
        static void read_complete(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf);

        void write(const char* buf, size_t bufsz);

    public:
        console(uv_loop_t*);
        console(const console&) = delete;
        console(console&&) = delete;
        ~console();

        static console& get() { return *m_instance; }
        static console& init(uv_loop_t* loop)
        {
            console* c = new console(loop);
            return *c;
        }

        void add_command(const console_cmd& cmd)
        {
            m_commands[cmd.m_command] = cmd;
        }

        void add_command(const ST::string& cmd, const ST::string& usage, const ST::string& desc, console_handler_t func)
        {
            m_commands[cmd] = { cmd, usage, desc, func };
        }

        template<size_t _CmdSz, size_t _UsageSz, size_t _DescSz>
        void add_command(const char(*cmd)[_CmdSz], const char(*usage)[_UsageSz], const char(*desc)[_DescSz], console_handler_t func)
        {
            m_commands[ST::string::from_literal(cmd, _CmdSz - 1)] = { ST::string::from_literal(cmd, _CmdSz - 1),
                                                                      ST::string::from_literal(usage, _UsageSz - 1),
                                                                      ST::string::from_literal(desc, _DescSz - 1),
                                                                      func };
        }

        void begin();
        void end();

        void set_prompt(const ST::string&);
        std::tuple<int, int> size();

        // Output stuff, emulates the std::ostream API
    protected:
        void output_ansi();

    public:
        template<typename T>
        console& operator <<(T value)
        {
            if (m_flags & e_outputAnsiDirty) {
                output_ansi();
                m_flags &= ~e_outputAnsiDirty;
                m_flags |= e_outputAnsiChanged;
            }
            m_outputBuf.operator<<(value);
            return *this;
        }

        console& operator <<(std::string_view value)
        {
            if (m_flags & e_outputAnsiDirty) {
                output_ansi();
                m_flags &= ~e_outputAnsiDirty;
                m_flags |= e_outputAnsiChanged;
            }
            m_outputBuf.append(value.data(), value.size());
            return *this;
        }

        console& operator <<(console&(*func)(console&))
        {
            return func(*this);
        }

        static console& endl(console& c)
        {
            c.m_outputBuf << '\n';
            flush(c);
            c.m_outputBuf.truncate();
            return c;
        }

        static console& flush(console& c);

#define COLOR_METHOD(prefix, member, color_name) \
    static console& prefix##_##color_name(console& c) \
    { \
        c.member = color::e_##color_name; \
        c.m_flags |= e_outputAnsiDirty; \
        return c; \
    }
        COLOR_METHOD(foreground, m_outForeColor, default)
        COLOR_METHOD(foreground, m_outForeColor, black)
        COLOR_METHOD(foreground, m_outForeColor, red)
        COLOR_METHOD(foreground, m_outForeColor, green)
        COLOR_METHOD(foreground, m_outForeColor, yellow)
        COLOR_METHOD(foreground, m_outForeColor, blue)
        COLOR_METHOD(foreground, m_outForeColor, magenta)
        COLOR_METHOD(foreground, m_outForeColor, cyan)
        COLOR_METHOD(foreground, m_outForeColor, white)

        COLOR_METHOD(background, m_outBackColor, default)
        COLOR_METHOD(background, m_outBackColor, black)
        COLOR_METHOD(background, m_outBackColor, red)
        COLOR_METHOD(background, m_outBackColor, green)
        COLOR_METHOD(background, m_outBackColor, yellow)
        COLOR_METHOD(background, m_outBackColor, blue)
        COLOR_METHOD(background, m_outBackColor, magenta)
        COLOR_METHOD(background, m_outBackColor, cyan)
        COLOR_METHOD(background, m_outBackColor, white)
#undef COLOR_METHOD

#define WEIGHT_METHOD(weight_name) \
    static console& weight_##weight_name(console& c) \
    { \
        c.m_outWeight = weight::e_##weight_name; \
        c.m_flags |= e_outputAnsiDirty; \
        return c; \
    }
        WEIGHT_METHOD(normal)
        WEIGHT_METHOD(bold)
#undef WEIGHT_METHOD

    };
};

#endif
