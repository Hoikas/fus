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

#include <algorithm>
#include "console.h"
#include "core/build_info.h"
#include "core/errors.h"
#include <cstring>
#include <vector>

// =================================================================================

fus::console* fus::console::m_instance = nullptr;

// =================================================================================

fus::console::console(uv_loop_t* loop)
    : m_charBuf(), m_flags(),  m_lastBell(), m_cursorPos(), m_historyIt(m_history.cend()), m_inputCharacters(),
      m_outForeColor(color::e_default), m_outBackColor(color::e_default), m_outWeight(weight::e_normal)
{
    FUS_ASSERTD(!m_instance);
    m_instance = this;

    uv_tty_init(loop, &m_stdin, 0, 1);
    uv_tty_init(loop, &m_stdout, 1, 0);

    uv_handle_set_data((uv_handle_t*)&m_stdin, this);
    uv_handle_set_data((uv_handle_t*)&m_stdout, this);

    // Make a nice prompt... If we have a tag it's probably something like "v1.00" -- so, let's
    // chop off the leading alpha character if the second one is numeric.
    const char* tag = fus::build_tag();
    const char* hash = fus::build_hash();
    if (tag && *tag) {
        if (isalpha(tag[0]) && isdigit(tag[1]))
            set_prompt(ST::format("fus-{}> ", tag + 1));
        else
            set_prompt(ST::format("fus-{}> ", tag));
    } else if (hash && *hash) {
        set_prompt(ST::format("fus-{}> ", hash));
    } else {
        set_prompt(ST_LITERAL("fus> "));
    }

    // Built-in help command
    add_command("help", "help [command]", "Displays help for a given console command", print_help);
}

fus::console::~console()
{
    uv_close((uv_handle_t*)&m_stdin, nullptr);
    uv_close((uv_handle_t*)&m_stdout, nullptr);
    uv_tty_reset_mode();
    m_instance = nullptr;
}

// =================================================================================

size_t fus::console::count_bytes(const char* buf, size_t bufsz, size_t charcount)
{
    const char* ptr = buf;
    const char* endp = buf + bufsz;
    size_t bytecount = 0;
    for (size_t i = 0; i < charcount && ptr < endp; ++i) {
        if (*ptr <= 0x7F)
            bytecount++;
        else if ((*ptr & 0xE0) == 0xC0)
            bytecount += 2;
        else if ((*ptr & 0xF0) == 0xE0)
            bytecount += 3;
        else if ((*ptr & 0xF8) == 0xF0)
            bytecount += 4;
        ptr = buf + bytecount;
    }
    return bytecount;
}

size_t fus::console::count_characters(const char* buf, size_t bufsz)
{
    size_t result = 0;
    for (size_t i = 0; i < bufsz;) {
        if (buf[i] == 0) {
            break;
        } else if (buf[i] <= 0x7F) {
            i++;
        } else if ((buf[i] & 0xE0) == 0xC0) {
            i += 2;
        } else if ((buf[i] & 0xF0) == 0xE0) {
            i += 3;
        } else if ((buf[i] & 0xF8) == 0xF0) {
            i += 4;
        } else {
            FUS_ASSERTD(0);
            break;
        }
        result++;
    }
    return result;
}

// =================================================================================

bool fus::console::print_help(console& console, const ST::string& args)
{
    auto cmdIt = console.m_commands.find(args);
    if (cmdIt == console.m_commands.end()) {
        console << weight_bold << foreground_yellow << "Fus Console Help:\n" << weight_normal << foreground_white;
        for (auto cmd : console.m_commands) {
            console << "    " << cmd.first << " - " << cmd.second.m_desc << "\n";
        }
        console << flush;
    } else {
        console << "Usage: " << cmdIt->second.m_usage << endl;
    }
    return true;
}

// =================================================================================

void fus::console::ring_bell()
{
    uint64_t nowMs = uv_now(uv_handle_get_loop((uv_handle_t*)&m_stdout));
    uint64_t elapsed = nowMs - m_lastBell;
    if (elapsed > 500) {
        m_lastBell = nowMs;
        write("\x07", 1);
    }
}

void fus::console::history_up(unsigned int value)
{
    unsigned int range = m_historyIt - m_history.cbegin();
    unsigned int seek = std::min(range, value);
    if (seek != value)
        ring_bell();
    m_historyIt -= seek;
    set_input_line(*m_historyIt);
}

void fus::console::history_down(unsigned int value)
{
    if (m_historyIt == m_history.cend()) {
        ring_bell();
        return;
    }
    unsigned int range = m_history.cend() - m_historyIt;
    unsigned int seek = std::min(range, value);
    m_historyIt += seek;

    // Down is a bit different in that when we reach the end, we'll just use an empty line.
    if (m_historyIt == m_history.cend())
        set_input_line(ST::null);
    else
        set_input_line(*m_historyIt);
}


void fus::console::move_cursor_left(unsigned int value)
{
    signed int request = m_cursorPos - value;
    if (request < 0)
        ring_bell();
    unsigned int seek = std::min(value, m_cursorPos);
    m_cursorPos -= seek;

    // Pass the result on to libuv
    if (seek > 1) {
        ST::string ansi = ST::format("\x1B[{}D", seek);
        write(ansi.c_str(), ansi.size());
    } else if (seek == 1) {
        write("\x1B[D", 3);
    }
}

void fus::console::move_cursor_right(unsigned int value)
{
    unsigned int request = m_cursorPos + value;
    if (request > m_inputCharacters)
        ring_bell();
    unsigned int seek = std::min(m_inputCharacters - m_cursorPos, value);
    m_cursorPos += seek;

    // Pass the result on to libuv
    if (seek > 1) {
        ST::string ansi = ST::format("\x1B[{}C", seek);
        write(ansi.c_str(), ansi.size());
    } else if (seek == 1) {
        write("\x1B[C", 3);
    }
}

void fus::console::handle_backspace()
{
    if (m_cursorPos == 0) {
        ring_bell();
    } else {
        delete_character();
    }
}

void fus::console::handle_delete()
{
    if (m_cursorPos == m_inputCharacters) {
        ring_bell();
    } else {
        // Danger: lazy...
        m_cursorPos++;
        write("\x1B[C", 3);
        delete_character();
    }
}

void fus::console::fire_command(const ST::string& cmd, const ST::string& args)
{
    auto cmdIt = m_commands.find(cmd);
    if (cmdIt == m_commands.end()) {
        *this << weight_bold << foreground_red << "Error: Unknown command '" << cmd << "'" << endl;
    } else {
        if (!cmdIt->second.m_handler(*this, args))
            *this << " Usage: " << cmdIt->second.m_usage;
    }
}

void fus::console::handle_input_line()
{
    // Regardless of the outcome of this, history has been rewritten.
    ST::string line = m_inputBuf.to_string();
    if (line.empty())
        return;
    m_history.push_back(line);
    while (m_history.size() > 200)
        m_history.pop_front();
    m_historyIt = m_history.cend();

    // Reset the display
    ST::string output = ST::format("\r\x1B[K{}{}\n", m_prompt, line);
    write(output.c_str(), output.size());

    // Reset internal state
    m_inputBuf.truncate();
    m_inputCharacters = 0;
    m_cursorPos = 0;

    // Supress the console input while a command is executing
    m_flags |= e_execCommand;

    // Fire off any valid command...
    std::vector<ST::string> cmd = line.split(' ', 1);
    if (!cmd.empty()) {
        fire_command(cmd[0], cmd.size() == 2 ? cmd[1] : ST::null);
    }

    // Implicitly write out the prompt
    m_flags &= ~e_execCommand;
    *this << flush;
}

void fus::console::handle_ctrl(const char* buf, size_t bufsz)
{
    if (*buf == 0)
        return;

    if (*buf == 0x1B) {
        // Limited processing so we can handle commands and stay in sync with the UI
        const char* ptr = buf + 1;
        FUS_ASSERTD(*ptr++ == '[');

        const char* cmd_ptr;
        unsigned long arg = std::max(strtoul(ptr, (char**)&cmd_ptr, 10), (unsigned long)1);

        if (*cmd_ptr == ';') {
            // This is some turd that no one gives a rip about
            return;
        } else if (*cmd_ptr == 'A') {
            history_up(arg);
        } else if (*cmd_ptr == 'B') {
            history_down(arg);
        } else if (*cmd_ptr == 'C') {
            move_cursor_right(arg);
        } else if (*cmd_ptr == 'D') {
            move_cursor_left(arg);
        } else if (*cmd_ptr == '~') {
            handle_delete();
        } else {
            // Kiss my rump.
            return;
        }
    } else if (*buf == 0x08 || *buf == 0x7F) {
        handle_backspace();
    } else if (*buf == '\r' || *buf == '\n') {
        handle_input_line();
    }
}

void fus::console::process_character(const char* buf, size_t bufsz)
{
    // Standard printable characters to the working line
    if ((*buf >= 0x20 && *buf <= 0x7E) || *buf > 0x7F) {
        insert_character(buf, bufsz);
    } else {
        handle_ctrl(buf, bufsz);
    }
}

// =================================================================================

void fus::console::delete_character()
{
    m_cursorPos--;
    m_inputCharacters--;

    if (m_inputCharacters != m_cursorPos) {
        // See insert_character() [written before this method] for diatribe...
        char* workBuf = const_cast<char*>(m_inputBuf.raw_buffer());
        size_t inputsz = m_inputBuf.size();

        size_t delPos = count_bytes(workBuf, inputsz, m_cursorPos);
        char* delBuf = workBuf + delPos;
        size_t delBytes = count_bytes(delBuf, inputsz - delPos, 1);
        size_t remsz = inputsz - delPos - delBytes;
        memmove(delBuf, delBuf + delBytes, remsz);
        m_inputBuf.truncate(inputsz - delBytes);

        // Move cursor back, rerender line
        write("\x1B[D", 3);
        write(delBuf, remsz);

        // Now, clear any junk left over and reset the cursor
        ST::string reset = ST::format("\x1B[K\x1B[{}D", m_inputCharacters - m_cursorPos);
        write(reset.c_str(), reset.size());
    } else {
        // Determine how many bytes the last character occupies
        size_t count = 0;
        const char* ptr = m_inputBuf.raw_buffer() + m_inputBuf.size();
        while (count < 4 && ptr != m_inputBuf.raw_buffer()) {
            count++;
            if ((*ptr-- & 0xC0) != 0x80)
                break;
        }
        m_inputBuf.truncate(m_inputBuf.size() - count);

        write("\x1B[D\x1B[K", 6);
    }
}

void fus::console::insert_character(const char* buf, size_t bufsz)
{
    m_inputBuf.append(buf, bufsz);

    if (m_inputCharacters != m_cursorPos) {
        // Need to shift the dam characters in the supposedly const memory. sigh.
        // Should be OK though. Underlying memory is not actually const.
        char* workBuf = const_cast<char*>(m_inputBuf.raw_buffer());
        size_t inputsz = m_inputBuf.size();

        // Need to memmove all the chars after the cursor...
        size_t cursorBytes = count_bytes(workBuf, inputsz, m_cursorPos);
        char* cursorBuf = workBuf + cursorBytes;
        memmove((cursorBuf + bufsz), cursorBuf, (inputsz - cursorBytes - bufsz));

        // Place the new character into the now open memory
        memcpy(cursorBuf, buf, bufsz);

        // We will unfortunately have to rerender the whole line from this point. THEN move the
        // damn cursor back to where the fucker came from. SIGH
        write(cursorBuf, inputsz - cursorBytes);

        ST::string shit = ST::format("\x1B[{}D", m_inputCharacters - m_cursorPos);
        write(shit.c_str(), shit.size());
    } else {
        // libuv's cursor is in the correct location, so we will simply pass along this update
        write(buf, bufsz);
    }

    // In the event of a paste operation, this will actually be multiple characters. Ugh.
    m_cursorPos += count_characters(buf, bufsz);
    m_inputCharacters  += count_characters(buf, bufsz);
}

void fus::console::flush_input_line()
{
    ST::string line = ST::format("\r\x1B[K{}{}", m_prompt, m_inputBuf.to_string());
    write(line.c_str(), line.size());
}

void fus::console::set_input_line(const ST::string& value)
{
    m_inputBuf.truncate();
    m_inputBuf << value;
    flush_input_line();
    m_inputCharacters = count_characters(value.c_str(), value.size());
    m_cursorPos = m_inputCharacters;
}

// =================================================================================

void fus::console::allocate(uv_stream_t* stream, size_t suggestion, uv_buf_t* buf)
{
    fus::console* console = (fus::console*)uv_handle_get_data((uv_handle_t*)stream);

    // If this doesn't work, kiss my rump.
    *buf = uv_buf_init((char*)&console->m_charBuf, sizeof(console->m_charBuf));
}

void fus::console::read_complete(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf)
{
    fus::console* console = (fus::console*)uv_handle_get_data((uv_handle_t*)stream);
    if (nread >= 0) {
        size_t idx = std::max((size_t)(buf->len - 1), (size_t)nread);
        buf->base[idx] = 0;
        console->process_character(buf->base, nread);
    }
}

// =================================================================================

struct console_write_t
{
    uv_write_t m_req;
    char m_buf[];
};

static void write_complete(console_write_t* req, int status)
{
    free(req);
}

void fus::console::write(const char* buf, size_t bufsz)
{
    // The console will (hopefully) immediately accept everything.
    uv_buf_t writebuf = uv_buf_init((char*)buf, bufsz);
    int writesz = uv_try_write((uv_stream_t*)&m_stdout, &writebuf, 1);

    // No data accepted. This is OK.
    if (writesz == UV_EAGAIN)
        writesz = 0;
    FUS_ASSERTD(writesz >= 0);

    // If any data remains to be written, enqueue that. :(
    if (writesz < bufsz) {
        size_t rem = bufsz - writesz;
        console_write_t* req = (console_write_t*)malloc(sizeof(console_write_t) + rem);
        memcpy(req->m_buf, buf + writesz, rem);
        writebuf = uv_buf_init(req->m_buf, rem);
        uv_write((uv_write_t*)req, (uv_stream_t*)&m_stdout, &writebuf, 1, (uv_write_cb)write_complete);
    }
}

// =================================================================================

void fus::console::output_ansi()
{
    m_outputBuf.append("\x1B[", 2);
    if (m_outWeight == weight::e_bold)
        m_outputBuf.append_char('1');
    else
        m_outputBuf.append_char('0');

    switch (m_outForeColor) {
    case color::e_black:
        m_outputBuf.append(";30", 3);
        break;
    case color::e_red:
        m_outputBuf.append(";31", 3);
        break;
    case color::e_green:
        m_outputBuf.append(";32", 3);
        break;
    case color::e_yellow:
        m_outputBuf.append(";33", 3);
        break;
    case color::e_blue:
        m_outputBuf.append(";34", 3);
        break;
    case color::e_magenta:
        m_outputBuf.append(";35", 3);
        break;
    case color::e_cyan:
        m_outputBuf.append(";36", 3);
        break;
    case color::e_white:
        m_outputBuf.append(";37", 3);
        break;
    }

    switch (m_outBackColor) {
    case color::e_black:
        m_outputBuf.append(";40", 3);
        break;
    case color::e_red:
        m_outputBuf.append(";41", 3);
        break;
    case color::e_green:
        m_outputBuf.append(";42", 3);
        break;
    case color::e_yellow:
        m_outputBuf.append(";43", 3);
        break;
    case color::e_blue:
        m_outputBuf.append(";44", 3);
        break;
    case color::e_magenta:
        m_outputBuf.append(";45", 3);
        break;
    case color::e_cyan:
        m_outputBuf.append(";46", 3);
        break;
    case color::e_white:
        m_outputBuf.append(";47", 3);
        break;
    }
    m_outputBuf.append_char('m');
}

fus::console& fus::console::flush(fus::console& c)
{
    // First, we clear the working line that the doofus is typing.
    c.write("\r\x1B[K", 4);

    // If the ANSI text mode was changed, we need to reset to something sensible.
    if (c.m_flags & e_outputAnsiChanged) {
        c.m_outputBuf << "\x1B[0m";
        c.m_flags &= ~e_outputAnsiChanged;
    }
    c.m_flags &= ~e_outputAnsiDirty;
    c.m_outBackColor = color::e_default;
    c.m_outForeColor = color::e_default;
    c.m_outWeight = weight::e_normal;

    // Flush the output
    c.write(c.m_outputBuf.raw_buffer(), c.m_outputBuf.size());
    c.m_outputBuf.truncate();

    // Write the working line back out.
    if (!(c.m_flags & e_execCommand))
        c.flush_input_line();

    return c;
}

// =================================================================================

void fus::console::begin()
{
    FUS_ASSERTD(!(m_flags & e_processing));

    m_flags |= e_processing;
    uv_tty_set_mode(&m_stdin, UV_TTY_MODE_RAW);
    uv_tty_set_mode(&m_stdout, UV_TTY_MODE_RAW);
    uv_read_start((uv_stream_t*)&m_stdin, (uv_alloc_cb)allocate, (uv_read_cb)read_complete);
    flush_input_line();
}

void fus::console::end()
{
    uv_read_stop((uv_stream_t*)&m_stdin);
    uv_tty_reset_mode();
}

void fus::console::shutdown()
{
    uv_unref((uv_handle_t*)&m_stdin);
    uv_unref((uv_handle_t*)&m_stdout);
}

void fus::console::set_prompt(const ST::string& value)
{
    m_prompt = value;
    m_promptSize = count_characters(value.c_str(), value.size());
}
