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
#include <iostream>

#include "errors.h"
#include "list.h"
#include "net_struct.h"
#include "tcp_stream.h"

// =================================================================================

constexpr size_t k_tooMuchMem = 10 * 1024 * 1024; // 10 MiB
constexpr size_t k_normBufsz = 1 * 1024 * 1024;   //  1 MiB
constexpr size_t k_tinyBufsz = 1 * 1024;          //  1 KiB

// =================================================================================

template<typename T>
static inline bool _alloc_buffer(T*& buf, size_t& bufsz, size_t alloc)
{
    // Don't allow some moron to request some huge @$$ buffer...
    if (alloc > k_tooMuchMem)
        return false;

    if (bufsz < alloc) {
        // Since we would like to reuse this buffer, we'll allocate a little extra memory.
        size_t blocks = alloc / sizeof(size_t);
        bufsz = (blocks + 2) * sizeof(size_t);
        T* temp = (T*)realloc(buf, bufsz);

        // Yikes... Might happen on something like an rpi though
        if (!temp) {
            free(buf);
            buf = nullptr;
            bufsz = 0;
            return false;
        }
        buf = temp;
    }
    return true;
}

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

static inline bool _is_any_buffer(const fus::net_struct_t* ns, size_t idx)
{
    const fus::net_field_t& field = ns->m_fields[idx];
    return field.m_type == fus::net_field_t::data_type::e_buffer ||
           field.m_type == fus::net_field_t::data_type::e_buffer_tiny ||
           field.m_type == fus::net_field_t::data_type::e_buffer_huge ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_tiny ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_huge;
}

static inline bool _is_redundant_buffer(const fus::net_struct_t* ns, size_t idx)
{
    const fus::net_field_t& field = ns->m_fields[idx];
    return field.m_type == fus::net_field_t::data_type::e_buffer_redundant ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_tiny ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_huge;
}

static inline bool _is_bufsz_legal(const fus::net_struct_t* ns, size_t idx, uint32_t bufsz)
{
    const fus::net_field_t& field = ns->m_fields[idx];
    switch (field.m_type) {
    case fus::net_field_t::data_type::e_buffer:
    case fus::net_field_t::data_type::e_buffer_redundant:
        return bufsz <= k_normBufsz;
    case fus::net_field_t::data_type::e_buffer_tiny:
    case fus::net_field_t::data_type::e_buffer_redundant_tiny:
        return bufsz <= k_tinyBufsz;
    case fus::net_field_t::data_type::e_buffer_huge:
    case fus::net_field_t::data_type::e_buffer_redundant_huge:
        return true;
    default:
        FUS_ASSERTD(0);
        return false;
    }
}

static inline uint32_t _determine_bufsz(const fus::net_struct_t* ns, size_t idx, const char* readbuf)
{
    // The last thing we read in was a 32-bit integer for the buffer size
    size_t szpos = fus::net_struct_calcsz(ns) - sizeof(uint32_t);
    uint32_t bufsz = *(const uint32_t*)(readbuf + szpos); /// fixme big endian

    // If the buffer is "redundant", that means it includes its size field in its size...
    if (_is_redundant_buffer(ns, idx))
        bufsz -= sizeof(uint32_t);
    return bufsz;
}

static void _read_alloc(fus::tcp_stream_t* stream, size_t suggestion, uv_buf_t* buf)
{
    // libuv pretty much always suggests 64KiB, according to its own "documentation"
    // So, let's examine the netstruct and see what we need to allocate.
    size_t alloc = 0;

    if (stream->m_readStruct) {
        alloc = fus::net_struct_calcsz(stream->m_readStruct);

        // When a read is executed, the size of any variable length field only accounts for the size portion.
        // Meaning that, in a best case scenario, we read once for the main body of the message and the
        // size of the buffer. Then, we have to read again for the buffer itself. Let's see if that's us.
        if (_is_any_buffer(stream->m_readStruct, stream->m_readField)) {
            uint32_t bufsz = _determine_bufsz(stream->m_readStruct, stream->m_readField, stream->m_readBuf);

            // Verify the client supplied buffer size is within acceptable bounds
            if (_is_bufsz_legal(stream->m_readStruct, stream->m_readField, bufsz))
                alloc += bufsz;
            else
                stream->m_flags |= fus::tcp_stream_t::e_readAllocFailed;
        } else {
            // Worst case... `m_readField == m_readStruct->m_size` when reading a buffer.
            FUS_ASSERTD((stream->m_readField < stream->m_readStruct->m_size));
        }
    } else {
        alloc = stream->m_readField;
    }

    // If the client tries to send us something too big, we just won't allocate enough memory.
    if (!_alloc_buffer(stream->m_readBuf, stream->m_readBufsz, alloc))
        stream->m_flags |= fus::tcp_stream_t::e_readAllocFailed;

    // Providing a null buffer on allocation fail causes libuv to return UV_ENOBUFS
    if (stream->m_flags & fus::tcp_stream_t::e_readAllocFailed)
        *buf = uv_buf_init(nullptr, 0);

    // We could have stopped reading in the middle of the message. Need to offset into the buffer.
    size_t offset = 0;
    if (stream->m_readStruct) {
        for (size_t i = stream->m_readField; i > 0;) {
            i--;
            offset += stream->m_readStruct->m_fields[i].m_datasz;
        }
    }

    FUS_ASSERTD(alloc >= offset);
    *buf = uv_buf_init((stream->m_readBuf + offset), (alloc - offset));
}

static void _read_complete(fus::tcp_stream_t* stream, ssize_t nread, uv_buf_t* buf)
{
    // Error cases:
    // 1) libuv error indicated by negative read size
    // 2) raw buffer read where nread != size reqested. we will consider that an EOF.
    // 3) client tried to send us a buffer that's too big -- we refused to allocate space for it
    //    note this should cause `nread == UV_ENOBUFS`, but we check for it anyway...
    if (nread < 0 || (!stream->m_readStruct && nread != stream->m_readField) || stream->m_flags & fus::tcp_stream_t::e_readAllocFailed) {
        stream->m_flags &= ~fus::tcp_stream_t::e_readAllocFailed;

        // Don't null the callback after calling it. The callback might reset the callback!
        stream->m_flags |= fus::tcp_stream_t::e_readCallback;
        fus::tcp_read_cb cb = nullptr;
        std::swap(cb, stream->m_readcb);
        std::cerr << uv_strerror(nread);
        cb(stream, nread < 0 ? nread : -1, nullptr);
        stream->m_flags &= ~fus::tcp_stream_t::e_readCallback;

        if (!(stream->m_flags & fus::tcp_stream_t::e_readQueued)) {
            stream->m_flags &= ~fus::tcp_stream_t::e_reading;
            uv_read_stop((uv_stream_t*)stream);
        } else {
            stream->m_flags &= ~fus::tcp_stream_t::e_readQueued;
        }
        return;
    }

    // Nonerror condition, continue reading...
    if (nread == 0)
        return;

    // Determine how many fields we read in
    if (stream->m_readStruct) {
#if 0
        // If we're at the last field+1 already, this is potentially a buffer that we finished reading.
        // Otherwise, we need to determine how many fields we read in.
        if (stream->m_readField != stream->m_readStruct->m_size) {
            for (ssize_t count = nread; count > 0; stream->m_readField++) {
                count -= stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
                /// fixme: more robost error handling here
                FUS_ASSERTD(count > -1);
                FUS_ASSERTD(stream->m_readField <= stream->m_readStruct->m_size);
            }
        }
#endif
        // How many fields did we read in?
        for (ssize_t count = nread; count > 0; stream->m_readField++) {
            // This implies a buffer can be anywhere in the struct. Don't lie to yourself--that should
            // never, never, never ever happen. EVER. The End.
            if (_is_any_buffer(stream->m_readStruct, stream->m_readField))
                count -= _determine_bufsz(stream->m_readStruct, stream->m_readField, stream->m_readBuf);
            else
                count -= stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
            FUS_ASSERTD(count > -1);
            FUS_ASSERTD(stream->m_readField < stream->m_readStruct->m_size);
        }


        // Dream on...
        if (stream->m_readField != stream->m_readStruct->m_size)
            return;
    }

    // Some code does actually care about the read size, I know...
    ssize_t bufsz = 0;
    if (stream->m_readStruct) {
        bufsz = fus::net_struct_calcsz(stream->m_readStruct);
        for (size_t i = 0; i < stream->m_readStruct->m_size; ++i) {
            if (_is_any_buffer(stream->m_readStruct, i))
                bufsz += _determine_bufsz(stream->m_readStruct, i, stream->m_readBuf);
        }
    } else {
        bufsz = stream->m_readField;
    }

#define FUS_IO_DEBUG_READS 1
#ifdef FUS_IO_DEBUG_READS
    if (stream->m_readStruct)
        fus::net_msg_print(stream->m_readStruct, stream->m_readBuf, std::cout);
#endif

    // Reset the read struct and read field anyway. Trying to use those might cause a buffer overrun.
    stream->m_readStruct = nullptr;
    stream->m_readField = 0;

    // Don't null the callback after calling it. The callback might reset the callback!
    stream->m_flags |= fus::tcp_stream_t::e_readCallback;
    fus::tcp_read_cb cb = nullptr;
    std::swap(cb, stream->m_readcb);
    cb(stream, bufsz, stream->m_readBuf);
    stream->m_flags &= ~fus::tcp_stream_t::e_readCallback;

    // If a read was not queued by the callback, stop this read.
    if (!(stream->m_flags & fus::tcp_stream_t::e_readQueued)) {
        stream->m_flags &= ~fus::tcp_stream_t::e_reading;
        uv_read_stop((uv_stream_t*)stream);
    } else {
        stream->m_flags &= ~fus::tcp_stream_t::e_readQueued;
    }
}

void fus::tcp_stream_read(fus::tcp_stream_t* stream, size_t bufsz, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(!(stream->m_flags & tcp_stream_t::e_readQueued));
    FUS_ASSERTD(bufsz);
    FUS_ASSERTD(read_cb);

    stream->m_readStruct = nullptr;
    stream->m_readField = bufsz;
    stream->m_readcb = read_cb;

    // Reissuing uv_read_start results in UV_EFAULT going to the provided read_cb
    if (stream->m_flags & tcp_stream_t::e_reading) {
        FUS_ASSERTD((stream->m_flags & tcp_stream_t::e_readCallback));
        stream->m_flags |= tcp_stream_t::e_readQueued;
    } else {
        stream->m_flags |= tcp_stream_t::e_reading;
        uv_read_start((uv_stream_t*)stream, (uv_alloc_cb)_read_alloc, (uv_read_cb)_read_complete);
    }
}

void fus::tcp_stream_read_msg(fus::tcp_stream_t* stream, const fus::net_struct_t* ns, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(!(stream->m_flags & tcp_stream_t::e_readQueued));
    FUS_ASSERTD(ns);
    FUS_ASSERTD(ns->m_size);
    FUS_ASSERTD(read_cb);

    stream->m_readStruct = ns;
    stream->m_readField = 0;
    stream->m_readcb = read_cb;

    // Reissuing uv_read_start results in UV_EFAULT going to the provided read_cb
    if (stream->m_flags & tcp_stream_t::e_reading) {
        FUS_ASSERTD((stream->m_flags & tcp_stream_t::e_readCallback));
        stream->m_flags |= tcp_stream_t::e_readQueued;
    } else {
        stream->m_flags |= tcp_stream_t::e_reading;
        uv_read_start((uv_stream_t*)stream, (uv_alloc_cb)_read_alloc, (uv_read_cb)_read_complete);
    }
}

// =================================================================================

struct write_buf_t
{
    uv_write_t m_req;
    size_t m_bufsz;
    uint8_t m_buf[]; // chicanery
};

static inline write_buf_t* _get_write_req(const void* buf, size_t bufsz)
{
    /// FIXME: allocations
    write_buf_t* req = (write_buf_t*)malloc(sizeof(write_buf_t) + bufsz);
    req->m_bufsz = bufsz;
    memcpy(req->m_buf, buf, bufsz);
    return req;
}

static void _write_complete(write_buf_t* req, int status)
{
    fus::tcp_write_cb cb = (fus::tcp_write_cb)uv_req_get_data((uv_req_t*)req);
    if (cb)
        cb((fus::tcp_stream_t*)req->m_req.handle, status < 0 ? status : req->m_bufsz);
    free(req);
}

void fus::tcp_stream_write(fus::tcp_stream_t* stream, const void* buf, size_t bufsz, tcp_write_cb write_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    write_buf_t* req = _get_write_req(buf, bufsz);
    uv_req_set_data((uv_req_t*)req, write_cb);
    uv_buf_t uvbuf = uv_buf_init((char*)req->m_buf, req->m_bufsz);
    uv_write((uv_write_t*)req, (uv_stream_t*)stream, &uvbuf, 1, (uv_write_cb)_write_complete);
}
