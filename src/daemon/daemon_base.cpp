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

#include <openssl/bn.h>
#include <string_theory/st_codecs.h>
#include <string_theory/st_format.h>

#include "core/errors.h"
#include "daemon_base.h"
#include "io/crypt_stream.h"
#include "protocol/common.h"
#include "server.h"

// =================================================================================

void fus::daemon_init(fus::daemon_t* daemon)
{
    const fus::config_parser& config = server::get()->config();
    daemon->m_buildId = config.get<unsigned int>("client", "buildId");
    daemon->m_branchId = config.get<unsigned int>("client", "branchId");
    daemon->m_buildType = config.get<unsigned int>("client", "buildType");
    FUS_ASSERTD(daemon->m_product.from_string(config.get<const ST::string&>("client", "product")));

    ST::string level = config.get<const ST::string&>("client", "verification").to_lower();
    if (level == ST_LITERAL("none"))
        daemon->m_verification = client_verification::e_none;
    else if (level == ST_LITERAL("strict"))
        daemon->m_verification = client_verification::e_strict;
    else
        daemon->m_verification = client_verification::e_default;
}

bool fus::daemon_verify_connection(const daemon_t* daemon, const void* msgbuf, bool permissive)
{
    if (daemon->m_verification == client_verification::e_none)
        return true;

    const protocol::connection_header* header = (const protocol::connection_header*)msgbuf;
    if (!permissive) {
        if (header->get_branchId() != daemon->m_branchId)
            return false;
        if (header->get_buildId() != daemon->m_buildId)
            return false;
    }
    if (daemon->m_verification == client_verification::e_strict || !permissive)
        if (!header->get_product()->equals(daemon->m_product))
            return false;
    return true;
}

// =================================================================================

static void _load_key(const ST::string& srv, const ST::string& key, BIGNUM* bn)
{
    const fus::config_parser& config = fus::server::get()->config();
    const ST::string& key_base64 = config.get<const ST::string&>(ST_LITERAL("crypt"), ST::format("{}_{}", srv, key));
    uint8_t key_data[64];
    FUS_ASSERTD(ST::base64_decode(key_base64, key_data, sizeof(key_data)) == sizeof(key_data));
    BN_bin2bn(key_data, sizeof(key_data), bn);
}

void fus::secure_daemon_init(fus::secure_daemon_t* daemon, const ST::string& srv)
{
    daemon_init((daemon_t*)daemon);

    daemon->m_bnK = BN_new();
    daemon->m_bnN = BN_new();

    _load_key(srv, ST_LITERAL("k"), daemon->m_bnK);
    _load_key(srv, ST_LITERAL("n"), daemon->m_bnN);
    FUS_ASSERTD(!BN_is_zero(daemon->m_bnN));
}

void fus::secure_daemon_close(fus::secure_daemon_t* daemon)
{
    BN_free(daemon->m_bnK);
    BN_free(daemon->m_bnN);
}

// =================================================================================

void fus::secure_daemon_encrypt_stream(fus::secure_daemon_t* daemon, fus::crypt_stream_t* stream, fus::crypt_established_cb cb)
{
    FUS_ASSERTD(daemon);
    FUS_ASSERTD(stream);

    crypt_stream_init(stream);
    crypt_stream_set_keys_server(stream, daemon->m_bnK, daemon->m_bnN);
    crypt_stream_establish_server(stream, cb);
}
