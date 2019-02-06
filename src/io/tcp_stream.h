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

#ifndef __FUS_TCP_STREAM_H
#define __FUS_TCP_STREAM_H

#include <string_theory/string>
#include <uv.h>

namespace fus
{
    struct tcp_stream_t;
    typedef void (*tcp_read_cb)(tcp_stream_t*, ssize_t, void*);
    typedef void (*tcp_write_cb)(tcp_stream_t*, ssize_t);

    struct tcp_stream_t
    {
        enum
        {
            // TCP Stream Flags
            e_reading = (1<<0),
            e_readQueued = (1<<1),
            e_readCallback = (1<<2),
            e_readAllocFailed = (1<<3),
            e_closing = (1<<4),
            e_freeOnClose = (1<<5),
            e_connected = (1<<6),

            // Crypt Stream Flags
            e_encrypted = (1<<7),
            e_ownSeed = (1<<8),
            e_ownKeys = (1<<9),
            e_hasSrvKeys = (1<<10),
            e_hasCliKeys = (1<<11),
            e_ownSrvKeysMask = e_ownKeys | e_hasSrvKeys,
        };

        uv_tcp_t m_tcp;
        uint32_t m_flags;

        const struct net_struct_t* m_readStruct;
        size_t m_readField;
        char* m_readBuf;
        size_t m_readBufsz;
        tcp_read_cb m_readcb;
        uv_close_cb m_closecb;
        uv_close_cb m_shutdowncb;
    };

    int tcp_stream_init(tcp_stream_t*, uv_loop_t*, unsigned int flags=0);
    void tcp_stream_free(tcp_stream_t*);
    void tcp_stream_shutdown(tcp_stream_t*, uv_close_cb cb=nullptr);

    int tcp_stream_accept(fus::tcp_stream_t* server, fus::tcp_stream_t* client);

    void tcp_stream_close_cb(tcp_stream_t*, uv_close_cb);
    void tcp_stream_free_on_close(tcp_stream_t*, bool);

    bool tcp_stream_connected(const tcp_stream_t*);
    ST::string tcp_stream_peeraddr(const tcp_stream_t*);

    void tcp_stream_read(tcp_stream_t*, size_t msgsz, tcp_read_cb read_cb);
    void tcp_stream_read_struct(tcp_stream_t*, const struct net_struct_t*, tcp_read_cb read_cb);

    template<typename T>
    inline void tcp_stream_read_msg(tcp_stream_t* s, tcp_read_cb read_cb)
    {
        tcp_stream_read_struct(s, T::net_struct, read_cb);
    }

    void tcp_stream_write(tcp_stream_t*, const void*, size_t, tcp_write_cb write_cb=nullptr);
};

#endif
