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

#include "tcp_stream.h"

#include "errors.h"
#include "net_struct.h"

enum
{
    e_reading = (1<<0),
};

// =================================================================================

int fus::tcp_stream_init(fus::tcp_stream_t* stream, uv_loop_t* loop)
{
    // I realize the arguments are inverted. This is to better mirror the intention of the function.
    int result = uv_tcp_init(loop, (uv_tcp_t*)stream);
    if (result < 0)
        return result;

    // Zero init the fus structure
    stream->m_flags = 0;
    stream->m_readStruct = nullptr;
    stream->m_readField = 0;
    stream->m_readBuf = nullptr;
    stream->m_readBufsz = 0;
    stream->m_readcb = nullptr;
    return 0;
}

void fus::tcp_stream_close(fus::tcp_stream_t* stream, uv_close_cb close_cb)
{
    uv_close((uv_handle_t*)stream, close_cb);
}

// =================================================================================

static void _read_alloc(fus::tcp_stream_t* stream, size_t suggestion, uv_buf_t* buf)
{
    // libuv pretty much always suggests 64KiB, according to its own "documentation"
    // So, let's examine the netstruct and see what we need to allocate.
    size_t alloc = fus::net_struct_calcsz(stream->m_readStruct);

    // When a read is executed, the size of any variable length field only accounts for the size portion.
    // Meaning that, in a best case scenario, we read once for the main body of the message and the
    // size of the buffer. Then, we have to read again for the buffer itself. Let's see if that's us.
    if (stream->m_readStruct->m_fields[stream->m_readField].m_type == fus::net_field_t::data_type::e_buffer) {
        FUS_ASSERTD(stream->m_readField == stream->m_readStruct->m_size);
        FUS_ASSERTD(stream->m_readBuf);

        // The last thing we read in was a 32-bit integer for the buffer size
        size_t szpos = fus::net_struct_calcsz(stream->m_readStruct) - sizeof(uint32_t) - 1;
        uint32_t bufsz = *(uint32_t*)(stream->m_readBuf + szpos); /// fixme big endian
        alloc += bufsz;
    }

    /// TODO: evaluate a more appropriate allocation method. Currently, there is no sanity checking
    ///       for idiots connecting and trying to send a buffer of several gigs. Also, need to look
    ///       into releasing or resizing overlarge buffers for memory limited environments (rpi?)
    if (stream->m_readBuf) {
        if (stream->m_readBufsz < alloc) {
            // Since we would like to reuse this buffer, we'll allocate a little extra memory.
            size_t blocks = alloc / sizeof(size_t);
            size_t reallyalloc = (blocks + 2) * sizeof(size_t);

            stream->m_readBufsz = reallyalloc;
            stream->m_readBuf = (char*)realloc(stream->m_readBuf, reallyalloc);
        }
    } else {
        // Since we would like to reuse this buffer, we'll allocate a little extra memory.
        size_t blocks = alloc / sizeof(size_t);
        size_t reallyalloc = (blocks + 4) * sizeof(size_t);

        stream->m_readBuf = (char*)malloc(reallyalloc);
        stream->m_readBufsz = reallyalloc;
    }

    // We could have stopped reading in the middle of the message. Need to offset into the buffer.
    size_t offset = 0;
    for (size_t i = stream->m_readField; i != 0; i--)
        offset += stream->m_readStruct->m_fields[i].m_datasz;

    // This isn't what libuv asked for, but it's what it deserves.
    *buf = uv_buf_init((stream->m_readBuf + offset), (alloc - offset));
}

static void _read_complete(fus::tcp_stream_t* stream, ssize_t nread, uv_buf_t* buf)
{
    /// fixme: assuming success
    if (nread < 0) {
        uv_read_stop((uv_stream_t*)stream);
        stream->m_readStruct = nullptr;
        stream->m_readField = 0;
        stream->m_flags &= ~e_reading;
        stream->m_readcb = nullptr;
        return;
    }

    // Determine how many fields we read in
    for (ssize_t count = nread; count > 0; stream->m_readField++) {
        count -= stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
        /// fixme: more robost error handling here
        FUS_ASSERTD(count > -1);
    }

    // If we have finished reading the struct, we need to fire off our read callback
    // fixme: we might have read all of the fields but still need to read the variable length
    //        buffer portion of the last field. no buffers supported yet, so who cares atm.
    if (stream->m_readField == stream->m_readStruct->m_size) {
        uv_read_stop((uv_stream_t*)stream);
        stream->m_readStruct = nullptr;
        stream->m_readField = 0;
        stream->m_flags &= ~e_reading;

        // Don't null the callback after calling it. The callback might reset the callback!
        fus::read_msg_cb cb = nullptr;
        std::swap(cb, stream->m_readcb);
        cb(stream, 0, stream->m_readBuf);
    }
}

void fus::tcp_stream_read_msg(fus::tcp_stream_t* stream, const fus::net_struct_t* ns, fus::read_msg_cb read_cb)
{
    FUS_ASSERTD(!(stream->m_flags & e_reading));
    FUS_ASSERTD(ns);
    FUS_ASSERTD(ns->m_size);
    FUS_ASSERTD(read_cb);

    stream->m_flags |= e_reading;
    stream->m_readStruct = ns;
    stream->m_readField = 0;
    stream->m_readcb = read_cb;
    uv_read_start((uv_stream_t*)stream, (uv_alloc_cb)_read_alloc, (uv_read_cb)_read_complete);
}
