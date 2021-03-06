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

#include <cstring>
#include <iostream>

#include "core/endian.h"
#include "core/errors.h"
#include "crypt_stream.h" // https://www.youtube.com/watch?v=IvzFt8PPXvE
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
        if (temp)
            buf = temp;
        else
            return false;
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
    stream->m_closecb = nullptr;
    stream->m_freecb = nullptr;
    stream->m_refcount = 1;
    return 0;
}

void fus::tcp_stream_free(fus::tcp_stream_t* stream)
{
    if (--stream->m_refcount > 0)
        return;

    // This is safe because crypt_stream_t tracks its resources using our flags field
    fus::crypt_stream_free((fus::crypt_stream_t*)stream);

    free(stream->m_readBuf);
    free(stream);
}

// =================================================================================

void fus::tcp_stream_ref(fus::tcp_stream_t* stream, uv_handle_t* handle)
{
    uv_handle_set_data(handle, stream);
    stream->m_refcount++;
}

void fus::tcp_stream_unref(uv_handle_t* handle)
{
    fus::tcp_stream_t* stream = (fus::tcp_stream_t*)uv_handle_get_data(handle);
    // Unrefs and frees us, if applicable
    tcp_stream_free(stream);
}

// =================================================================================

static void _tcp_close(fus::tcp_stream_t* stream)
{
    stream->m_flags &= ~fus::tcp_stream_t::e_connected;
    stream->m_flags &= ~fus::tcp_stream_t::e_closing;

    if (stream->m_closecb)
        stream->m_closecb((uv_handle_t*)stream);
    if ((stream->m_flags & fus::tcp_stream_t::e_freeOnClose)) {
        // Our "subclasses" might have ref'd us, so we need to call its free method, which will
        // unref us in a defferred manner. We'll release our main ref as well.
        if (stream->m_freecb)
            stream->m_freecb(stream);
        fus::tcp_stream_free(stream);
    } else {
        // need to reset to a clean, reusable state
        fus::crypt_stream_free((fus::crypt_stream_t*)stream);
        uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)stream);
        FUS_ASSERTD(uv_tcp_init(loop, (uv_tcp_t*)stream) == 0);
        stream->m_flags = 0;
    }
}

static void _tcp_shutdown(uv_shutdown_t* req, int status)
{
    fus::tcp_stream_t* stream = (fus::tcp_stream_t*)req->handle;
    uv_close((uv_handle_t*)stream, (uv_close_cb)_tcp_close);
}

void fus::tcp_stream_shutdown(fus::tcp_stream_t* stream)
{
    FUS_ASSERTD(stream);

    stream->m_flags |= tcp_stream_t::e_closing;
    uv_shutdown(&stream->m_shutdown, (uv_stream_t*)stream, _tcp_shutdown);
}

// =================================================================================

int fus::tcp_stream_accept(fus::tcp_stream_t* server, fus::tcp_stream_t* client)
{
    int result = uv_accept((uv_stream_t*)server, (uv_stream_t*)client);
    if (result == 0) {
        uv_tcp_nodelay((uv_tcp_t*)client, 1);
        client->m_flags |= tcp_stream_t::e_connected;
    }
    return result;
}

// =================================================================================

void fus::tcp_stream_close_cb(fus::tcp_stream_t* stream, uv_close_cb cb)
{
    stream->m_closecb = cb;
}

void fus::tcp_stream_free_cb(fus::tcp_stream_t* stream, tcp_free_cb cb)
{
    stream->m_freecb = cb;
}

void fus::tcp_stream_free_on_close(fus::tcp_stream_t* stream, bool value)
{
    if (value)
        stream->m_flags |= tcp_stream_t::e_freeOnClose;
    else
        stream->m_flags &= ~tcp_stream_t::e_freeOnClose;
}

// =================================================================================

bool fus::tcp_stream_closing(const fus::tcp_stream_t* stream)
{
    return stream->m_flags & tcp_stream_t::e_closing;
}

bool fus::tcp_stream_connected(const fus::tcp_stream_t* stream)
{
    return stream->m_flags & tcp_stream_t::e_connected;
}

ST::string fus::tcp_stream_peeraddr(const fus::tcp_stream_t* stream)
{
    sockaddr_storage addr;
    int addrsz = sizeof(addr);
    uv_tcp_getpeername((const uv_tcp_t*)stream, (sockaddr*)(&addr), &addrsz);

    char addrstr[64] = {"???"};
    if (addr.ss_family == AF_INET)
        uv_ip4_name((sockaddr_in*)(&addr), addrstr, sizeof(addrstr));
    else if (addr.ss_family == AF_INET6)
        uv_ip6_name((sockaddr_in6*)(&addr), addrstr, sizeof(addrstr));
    return ST::format("{}/{}", addrstr, ((sockaddr_in*)&addr)->sin_port);
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
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_huge ||
           field.m_type == fus::net_field_t::data_type::e_string_utf16;
}

static inline bool _is_redundant_buffer(const fus::net_struct_t* ns, size_t idx)
{
    const fus::net_field_t& field = ns->m_fields[idx];
    return field.m_type == fus::net_field_t::data_type::e_buffer_redundant ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_tiny ||
           field.m_type == fus::net_field_t::data_type::e_buffer_redundant_huge;
}

static inline bool _is_string(const fus::net_struct_t* ns, size_t idx)
{
    const fus::net_field_t& field = ns->m_fields[idx];
    return field.m_type == fus::net_field_t::data_type::e_string_utf8 ||
           field.m_type == fus::net_field_t::data_type::e_string_utf16;
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
    case fus::net_field_t::data_type::e_string_utf16:
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
    FUS_ASSERTD(idx > 0);

    const char* szbuf = readbuf - ns->m_fields[idx-1].m_datasz;
    size_t bufsz;
    switch (ns->m_fields[idx-1].m_datasz) {
        case sizeof(uint8_t):
            bufsz = *(uint8_t*)szbuf;
            break;
        case sizeof(uint16_t):
            bufsz = FUS_LE16(*(uint16_t*)szbuf);
            break;
        case sizeof(uint32_t):
            bufsz = FUS_LE32(*(uint32_t*)szbuf);
            break;
        case sizeof(uint64_t):
            bufsz = FUS_LE64(*(uint64_t*)szbuf);
            break;
        default:
            FUS_ASSERTR(0);
            break;
    }

    // Cyan's wire format stores sizes as multiples of the data type. This is generally bytes,
    // but in some cases... no. NOTE that in fus we use byte counts.
    if (ns->m_fields[idx].m_type == fus::net_field_t::data_type::e_string_utf16)
        bufsz *= sizeof(char16_t);

    // If the buffer is "redundant", that means it includes its size field in its size...
    if (_is_redundant_buffer(ns, idx))
        bufsz -= ns->m_fields[idx-1].m_datasz;
    return bufsz;
}

static void _read_alloc(fus::tcp_stream_t* stream, size_t suggestion, uv_buf_t* buf)
{
    // libuv suggests 64KiB pretty much always according to both science and its own
    // so called "documentation". Previously, we tried to scan the entire netstruct and
    // allocate enough buffer space for the whole stupid thing. However, this was before
    // I remembered/realized strings are variable length and can appear anywhere. Therefore,
    // when we read, we will read and allocate one field at at time.
    size_t bufsz = 0;
    size_t alloc = 0;
    size_t offset = 0;

    if (stream->m_readStruct) {
        // Where did we leave off in our read?
        offset = fus::net_struct_calcsz(stream->m_readStruct, stream->m_readField);

        // If we have a buffer field, the field immediately preceeding us is the buffer size.
        // In the case of binary data buffers, we know they always exist at the end of the
        // message, so we only allocate enough space for them. However, if the buffer is a string,
        // it can occur anywhere in the message. We'll need to allocate its internal size but only
        // report the size to read in to libuv...
        if (_is_any_buffer(stream->m_readStruct, stream->m_readField)) {
            bufsz = _determine_bufsz(stream->m_readStruct, stream->m_readField, stream->m_readBuf + offset);
            if (_is_string(stream->m_readStruct, stream->m_readField)) {
                alloc = stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
                if (bufsz > alloc)
                    stream->m_flags |= fus::tcp_stream_t::e_readAllocFailed;
            } else {
                alloc = bufsz;
            }
            if (!_is_bufsz_legal(stream->m_readStruct, stream->m_readField, bufsz)) {
                stream->m_flags |= fus::tcp_stream_t::e_readAllocFailed;
            }
        } else {
            bufsz = stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
            alloc = stream->m_readStruct->m_fields[stream->m_readField].m_datasz;
        }
    } else {
        bufsz = stream->m_readField;
        alloc = stream->m_readField;
    }

    if (!(stream->m_flags & fus::tcp_stream_t::e_readAllocFailed)) {
        if (!_alloc_buffer(stream->m_readBuf, stream->m_readBufsz, offset + alloc))
            stream->m_flags |= fus::tcp_stream_t::e_readAllocFailed;
    }

    if (stream->m_flags & fus::tcp_stream_t::e_readAllocFailed) {
        *buf = uv_buf_init(nullptr, 0);
    } else {
        *buf = uv_buf_init(stream->m_readBuf + offset, bufsz);
    }
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

    // Before we do ANYTHING else... The alloc callback assumes it can peak into the buffer and see
    // decrypted contents. So, if this stream is encrypted, we need to decipher what we just read.
    if (stream->m_flags & fus::tcp_stream_t::e_encrypted) {
        FUS_ASSERTD(nread == buf->len);
        fus::crypt_stream_decipher((fus::crypt_stream_t*)stream, buf->base, nread);
    }

    // Determine how many fields we read in
    size_t structsz;
    if (stream->m_readStruct) {
        // Previously this was complicated code to count the number of fields we read in.
        stream->m_readField++;

        // If we're not at the end of the struct and the next field is a buffer or a string, that
        // field could be zero length. In that case, we need to advance past that field. This could
        // result in us reaching the end of the message, resulting in a dispatch, or require us
        // to keep going, in the case of strings...  If we don't handle this, the allocator will
        // refuse to allocate more buffer space for this message, and the read will fail with
        // UV_ENOBUFS...
        if (stream->m_readField < stream->m_readStruct->m_size) {
            size_t offset = fus::net_struct_calcsz(stream->m_readStruct, stream->m_readField);
            if (_is_any_buffer(stream->m_readStruct, stream->m_readField)) {
                size_t bufsz = _determine_bufsz(stream->m_readStruct, stream->m_readField,
                                                stream->m_readBuf + offset);
                if (bufsz == 0) {
                    stream->m_readField++;
                    if (stream->m_readField < stream->m_readStruct->m_size) {
                        // We're still in the middle of the struct...
                        return;
                    } else {
                        // Going to callback with the message size
                        structsz = offset + bufsz;
                    }
                } else {
                    // Need to read the buffer contents...
                    return;
                }
            } else {
                // Not a buffer, still need to read...
                return;
            }
        } else {
            // Done reading, need to ensure we have the total struct size.
            structsz = fus::net_struct_calcsz(stream->m_readStruct);
            if (_is_any_buffer(stream->m_readStruct, stream->m_readField - 1)) {
                structsz += _determine_bufsz(stream->m_readStruct, stream->m_readField - 1,
                                             stream->m_readBuf + structsz);
            }
        }
    } else {
        // We just read in some data off the wire.
        structsz = stream->m_readField;
    }

#ifdef FUS_IO_DEBUG_READS
    if (stream->m_readStruct)
        fus::net_msg_print(stream->m_readStruct, stream->m_readBuf, std::cout);
#endif

    // Reset the read struct and read field anyway. Trying to use those might cause a buffer overrun.
    stream->m_readStruct = nullptr;
    if (!(stream->m_flags & fus::tcp_stream_t::e_readPeek))
        stream->m_readField = 0;

    // Don't null the callback after calling it. The callback might reset the callback!
    stream->m_flags |= fus::tcp_stream_t::e_readCallback;
    fus::tcp_read_cb cb = nullptr;
    std::swap(cb, stream->m_readcb);
    cb(stream, structsz, stream->m_readBuf);
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
    stream->m_flags &= ~tcp_stream_t::e_readPeek;
    if (stream->m_flags & tcp_stream_t::e_reading) {
        FUS_ASSERTD((stream->m_flags & tcp_stream_t::e_readCallback));
        stream->m_flags |= tcp_stream_t::e_readQueued;
    } else {
        stream->m_flags |= tcp_stream_t::e_reading;
        uv_read_start((uv_stream_t*)stream, (uv_alloc_cb)_read_alloc, (uv_read_cb)_read_complete);
    }
}

void fus::tcp_stream_read_struct(fus::tcp_stream_t* stream, const fus::net_struct_t* ns, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(!(stream->m_flags & tcp_stream_t::e_readQueued));
    FUS_ASSERTD(ns);
    FUS_ASSERTD(ns->m_size);
    FUS_ASSERTD(read_cb);

    stream->m_readStruct = ns;
    if (!(stream->m_flags & tcp_stream_t::e_readPeek))
        stream->m_readField = 0;
    else
        FUS_ASSERTD(stream->m_readField < ns->m_size);
    stream->m_readcb = read_cb;

    // Reissuing uv_read_start results in UV_EFAULT going to the provided read_cb
    stream->m_flags &= ~tcp_stream_t::e_readPeek;
    if (stream->m_flags & tcp_stream_t::e_reading) {
        FUS_ASSERTD((stream->m_flags & tcp_stream_t::e_readCallback));
        stream->m_flags |= tcp_stream_t::e_readQueued;
    } else {
        stream->m_flags |= tcp_stream_t::e_reading;
        uv_read_start((uv_stream_t*)stream, (uv_alloc_cb)_read_alloc, (uv_read_cb)_read_complete);
    }
}

void fus::tcp_stream_peek_struct(fus::tcp_stream_t* stream, const fus::net_struct_t* ns, fus::tcp_read_cb read_cb)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(!(stream->m_flags & tcp_stream_t::e_readQueued));
    FUS_ASSERTD(ns);
    FUS_ASSERTD(ns->m_size);
    FUS_ASSERTD(read_cb);

    stream->m_readStruct = ns;
    if (!(stream->m_flags & tcp_stream_t::e_readPeek))
        stream->m_readField = 0;
    else
        FUS_ASSERTD(stream->m_readField < ns->m_size);
    stream->m_readcb = read_cb;

    // Reissuing uv_read_start results in UV_EFAULT going to the provided read_cb
    stream->m_flags |= tcp_stream_t::e_readPeek;
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
    char m_buf[]; // chicanery
};

static void _write_complete(write_buf_t* req, int status)
{
    free(req);
}

void fus::tcp_stream_write(fus::tcp_stream_t* stream, const void* buf, size_t bufsz)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(buf);
    FUS_ASSERTD(bufsz);

    if (!(stream->m_flags & tcp_stream_t::e_closing)) {
        write_buf_t* req = (write_buf_t*)malloc(sizeof(write_buf_t) + bufsz);
        req->m_bufsz = bufsz;
        if (stream->m_flags & fus::tcp_stream_t::e_encrypted)
            crypt_stream_encipher((crypt_stream_t*)stream, buf, req->m_buf, bufsz);
        else
            memcpy(req->m_buf, buf, bufsz);
        uv_buf_t uvbuf = uv_buf_init((char*)req->m_buf, req->m_bufsz);
        uv_write((uv_write_t*)req, (uv_stream_t*)stream, &uvbuf, 1, (uv_write_cb)_write_complete);
    }
}

void fus::tcp_stream_write_struct(fus::tcp_stream_t* stream, const fus::net_struct_t* ns,
                                  const void* buf, size_t bufsz,
                                  const void* appendBuf, size_t appendBufsz)
{
    FUS_ASSERTD(stream);
    FUS_ASSERTD(ns);
    FUS_ASSERTD(buf);

    if (!(stream->m_flags & tcp_stream_t::e_closing)) {
        /// FIXME: this allocation can be larger than needed.
        auto req = (write_buf_t*)malloc(sizeof(write_buf_t) + bufsz + appendBufsz);
        req->m_bufsz = 0;
        uv_buf_t* sendbufs = (uv_buf_t*)alloca(sizeof(uv_buf_t) * ns->m_size);
        const char* srcPtr = (const char*)buf;
        char* dstPtr = req->m_buf;

        size_t i;
        for (i = 0; i < ns->m_size; ++i) {
            sendbufs[i].base = dstPtr;
            if (_is_any_buffer(ns, i))
                sendbufs[i].len = _determine_bufsz(ns, i, srcPtr);
            else
                sendbufs[i].len = ns->m_fields[i].m_datasz;
            req->m_bufsz += sendbufs[i].len;

            // If this field is a variable length buffer, it is possible that the buffer we've
            // been provided does not contain this field. Therefore, we should make sure that
            // we actually have that data before charging forward.
            do {
                if (_is_any_buffer(ns, i) && !_is_string(ns, i) && (i + 1) == ns->m_size) {
                    size_t offset = (size_t)srcPtr - (size_t)buf;
                    if (offset == bufsz) {
                        // No data in the main buffer. Dang. Is the appendbuf acceptable?
                        // If no appendbuf provided, bail on this final buffer.
                        if (appendBuf) {
                            FUS_ASSERTD(appendBufsz == sendbufs[i].len);
                            srcPtr = (const char*)appendBuf;
                        } else {
                            i -= 1;
                            break;
                        }
                    } else {
                        // Data is present in the main buffer. Perhaps...
                        FUS_ASSERTD((bufsz - offset) == sendbufs[i].len);
                    }
                }

                if (stream->m_flags & fus::tcp_stream_t::e_encrypted)
                    crypt_stream_encipher((crypt_stream_t*)stream, srcPtr, dstPtr, sendbufs[i].len);
                else
                    memcpy(dstPtr, srcPtr, sendbufs[i].len);
            } while(0);

            dstPtr += sendbufs[i].len;
            srcPtr += ns->m_fields[i].m_datasz;
        }

        uv_write((uv_write_t*)req, (uv_stream_t*)stream, sendbufs, i, (uv_write_cb)_write_complete);
    }
}
