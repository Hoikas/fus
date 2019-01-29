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

#include "core/errors.h"
#include <filesystem>
#include "log_file.h"

// =================================================================================

static std::filesystem::path s_logdir;

static const std::filesystem::path& log_directory()
{
    if (s_logdir.empty())
        s_logdir = std::filesystem::current_path() / "log";
    return s_logdir;
}

void fus::log_file::set_directory(const ST::string& dir)
{
    FUS_ASSERTD(s_logdir.empty());
    s_logdir = dir.c_str();
}

// =================================================================================

int fus::log_file::open(uv_loop_t* loop, const ST::string& name)
{
    m_loop = loop;
    m_offset = 0;

    std::filesystem::path path = log_directory();
    std::error_code error;
    std::filesystem::create_directories(path, error);
    int test = error.value();
    path /= name.c_str();
    path.replace_extension(".log");

    // Perhaps we should implement log rotation at some point...
    uv_fs_t req;
    uv_file file = uv_fs_open(loop, &req, path.u8string().c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644, nullptr);
    uv_fs_req_cleanup(&req);
    if (file < 0) {
        m_file = -1;
        return file;
    } else {
        m_file = file;
        write("... Opened \"{}.log\"", name);
        return 0;
    }
}

void fus::log_file::close()
{
    uv_fs_t req;
    uv_fs_close(m_loop, &req, m_file, nullptr);
    uv_fs_req_cleanup(&req);
}

// =================================================================================

void fus::log_file::set_level(level level)
{
    m_level = level;
}

// =================================================================================

struct log_write_t
{
    uv_fs_t m_req;
    char m_buf[];
};

static void _log_write_complete(log_write_t* req)
{
    FUS_ASSERTD(uv_fs_get_result((uv_fs_t*)req) >= 0);
    uv_fs_req_cleanup((uv_fs_t*)req);
    free(req);
}

void fus::log_file::write(const ST::string& msg)
{
    if (m_file == -1)
        return;

    // Include the time in the log message
    /// TODO: should probably cache this every second or so
    time_t timest;
    time(&timest);
    tm* time = gmtime(&timest);
    char timestr[128];
    size_t timesz = strftime(timestr, sizeof(timestr), "[%Y-%m-%d %H:%M:%S] ", time);

    // Allocate a uv_fs_t with space for the log message and a newline character
    size_t bufsz = timesz + msg.size() + 1;
    size_t reqsz = sizeof(log_write_t) + bufsz;
    log_write_t* req = (log_write_t*)malloc(reqsz);
    uv_buf_t buf = uv_buf_init(req->m_buf, bufsz);

    // Copy the string data into the request and append a newline
    memcpy(req->m_buf, timestr, timesz);
    memcpy(req->m_buf + timesz, msg.c_str(), msg.size());
    *(req->m_buf + (bufsz-1)) = '\n'; // Don't look in the debugger...
    uv_fs_write(m_loop, (uv_fs_t*)req, m_file, &buf, 1, m_offset, (uv_fs_cb)_log_write_complete);

    // Because libuv insists on imitating pwritev
    m_offset += bufsz;
}
