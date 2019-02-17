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

#pragma pack(push,1)

#define FUS_NET_STRUCT_BEGIN(name) \
    namespace fus { namespace protocol { \
        struct name final { \
            static const fus::net_struct_t* net_struct;

#define FUS_NET_FIELD_BLOB(name, size) \
    uint8_t m_##name##[size]; \
    \
    uint32_t get_##name##sz() const { return size; } \
    const uint8_t* get_##name() const { return m_##name; } \
    uint8_t* get_##name() { return m_##name; }

#define FUS_NET_FIELD_BUFFER(name) \
    uint32_t m_##name##sz; \
    uint8_t m_##name[]; \
    \
    uint32_t get_##name##sz() const { return FUS_LE32(m_##name##sz); } \
    void set_##name##sz(uint32_t value) { m_##name##sz = FUS_LE32(value); } \
    const uint8_t* get_##name() const { return m_##name; } \
    uint8_t* get_##name() { return m_##name; }

#define FUS_NET_FIELD_BUFFER_TINY(name) FUS_NET_FIELD_BUFFER(name)
#define FUS_NET_FIELD_BUFFER_HUGE(name) FUS_NET_FIELD_BUFFER(name)

#define FUS_NET_FIELD_BUFFER_REDUNDANT(name) \
    uint32_t m_##name##sz; \
    uint8_t m_##name[]; \
    \
    uint32_t get_##name##sz() const { return FUS_LE32(m_##name##sz - sizeof(uint32_t)); } \
    void set_##name##sz(uint32_t value) { m_##name##sz = FUS_LE32(value + sizeof(uint32_t)); } \
    const uint8_t* get_##name() const { return m_##name; } \
    uint8_t* get_##name() { return m_##name; }

#define FUS_NET_FIELD_BUFFER_REDUNDANT_TINY(name) FUS_NET_FIELD_BUFFER_REDUNDANT(name)
#define FUS_NET_FIELD_BUFFER_REDUNDANT_HUGE(name) FUS_NET_FIELD_BUFFER_REDUNDANT(name)

#define FUS_NET_FIELD_UINT8(name) \
    uint8_t m_##name; \
    \
    uint8_t get_##name() const { return m_##name; } \
    void set_##name(uint8_t value) { m_##name = value; }

#define FUS_NET_FIELD_UINT16(name) \
    uint16_t m_##name; \
    \
    uint16_t get_##name() const { return FUS_LE16(m_##name); } \
    void set_##name(uint16_t value) { m_##name = FUS_LE16(value); }

#define FUS_NET_FIELD_UINT32(name) \
    uint32_t m_##name; \
    \
    uint32_t get_##name() const { return FUS_LE32(m_##name); } \
    void set_##name(uint32_t value) { m_##name = FUS_LE32(value); }

#define FUS_NET_FIELD_STRING(name, sz) \
    char16_t m_##name[sz]; \
    \
    ST::string get_##name() const { return ST::string(m_##name); } \
    void set_##name(const ST::string& value) \
    { \
        ST::buffer<char16_t> buf = value.to_utf16(); \
        memset(m_##name, 0, sz * sizeof(char16_t)); \
        size_t elmcopy = std::min((size_t)sz, buf.size()); \
        memcpy(m_##name, buf.data(), elmcopy * sizeof(char16_t)); \
        m_##name[sz - 1] = 0; \
    }

#define FUS_NET_FIELD_UUID(name) \
    uint8_t m_##name[16]; \
    const fus::uuid* get_##name() const { return (const fus::uuid*)m_##name; } \
    fus::uuid* get_##name() { return (fus::uuid*)m_##name; }

#define FUS_NET_STRUCT_END(name) \
    }; }; };
