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
            e_reading = (1<<0),
            e_readQueued = (1<<1),
            e_readCallback = (1<<2),
            e_readAllocFailed = (1<<3),
            e_encrypted = (1<<4),
        };

        uv_tcp_t m_tcp;
        uint32_t m_flags;

        const struct net_struct_t* m_readStruct;
        size_t m_readField;
        char* m_readBuf;
        size_t m_readBufsz;
        tcp_read_cb m_readcb;
    };

    int tcp_stream_init(tcp_stream_t*, uv_loop_t*);
    void tcp_stream_close(tcp_stream_t*, uv_close_cb close_cb=nullptr);
    void tcp_stream_read(tcp_stream_t*, size_t msgsz, tcp_read_cb read_cb);
    void tcp_stream_read_msg(tcp_stream_t*, const struct net_struct_t*, tcp_read_cb read_cb);
    void tcp_stream_write(tcp_stream_t*, const void*, size_t, tcp_write_cb write_cb=nullptr);
};

#endif
