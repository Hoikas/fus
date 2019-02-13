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

#include "client/db_client.h"
#include "core/errors.h"
#include "daemon_base.h"
#include "io/crypt_stream.h"
#include "protocol/common.h"
#include "server.h"

// =================================================================================

void fus::daemon_init(fus::daemon_t* daemon, const ST::string& name)
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

    daemon->m_log.set_level(server::get()->config().get<const ST::string&>("log", "level"));
    daemon->m_log.open(uv_default_loop(), name);

    daemon->m_flags = 0;
}

void fus::daemon_free(fus::daemon_t* daemon)
{
    FUS_ASSERTD((daemon->m_flags & daemon_t::e_shuttingDown));
    daemon->m_log.close();
}

void fus::daemon_shutdown(fus::daemon_t* daemon)
{
    daemon->m_flags |= daemon_t::e_shuttingDown;
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
    daemon_init((daemon_t*)daemon, ST::format("{}_daemon", srv));

    daemon->m_bnK = BN_new();
    daemon->m_bnN = BN_new();

    _load_key(srv, ST_LITERAL("k"), daemon->m_bnK);
    _load_key(srv, ST_LITERAL("n"), daemon->m_bnN);
    FUS_ASSERTD(!BN_is_zero(daemon->m_bnN));
}

void fus::secure_daemon_free(fus::secure_daemon_t* daemon)
{
    daemon_free((daemon_t*)daemon);

    BN_free(daemon->m_bnK);
    BN_free(daemon->m_bnN);
}

void fus::secure_daemon_shutdown(fus::secure_daemon_t* daemon)
{
    daemon_shutdown((daemon_t*)daemon);
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

// =================================================================================

static void db_connected(fus::db_client_t* db, ssize_t status)
{
    ST::string addr = fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db);
    fus::daemon_t* daemon = (fus::daemon_t*)uv_handle_get_data((uv_handle_t*)db);

    if (status == 0) {
        daemon->m_log.write_error("DB '{}' connection established!", addr);
        daemon->m_flags |= fus::daemon_t::e_dbConnected;
    } else if (daemon->m_flags & fus::daemon_t::e_shuttingDown) {
        daemon->m_log.write_info("DB '{}' connection abandoned", addr);
    } else {
        daemon->m_log.write_error("DB '{}' connection failed, retrying in 5s... Detail: {}",
                                  addr, uv_strerror(status));
        fus::client_reconnect((fus::client_t*)db, 5000);
    }
}

static void db_disconnected(fus::db_client_t* db)
{
    fus::daemon_t* daemon = (fus::daemon_t*)uv_handle_get_data((uv_handle_t*)db);
    if (daemon->m_flags & fus::daemon_t::e_shuttingDown) {
        daemon->m_log.write_info("DB connection shutdown");
        ((fus::db_trans_daemon_t*)daemon)->m_db = nullptr;
    } else {
        ST::string addr = fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db);
        daemon->m_log.write_error("DB '{}' connection lost, reconnecting in 5s...",
                                  fus::tcp_stream_peeraddr((fus::tcp_stream_t*)db));
        fus::client_reconnect((fus::client_t*)db, 5000);
    }
    daemon->m_flags &= ~fus::daemon_t::e_dbConnected;
}

// =================================================================================

void fus::db_trans_daemon_init(fus::db_trans_daemon_t* daemon, const ST::string& srv)
{
    secure_daemon_init((secure_daemon_t*)daemon, srv);

    daemon->m_db = (db_client_t*)malloc(sizeof(db_client_t));
    FUS_ASSERTD(db_client_init(daemon->m_db, uv_default_loop()) == 0);
    uv_handle_set_data((uv_handle_t*)daemon->m_db, daemon);
    tcp_stream_close_cb((tcp_stream_t*)daemon->m_db, (uv_close_cb)db_disconnected);
    {
        sockaddr_storage addr;
        server::get()->config2addr(ST_LITERAL("db"), &addr);

        auto header = (fus::protocol::connection_header*)alloca(db_client_header_size());
        header->set_msgsz(sizeof(fus::protocol::connection_header) - 4); // does not include the buf field
        header->set_buildId(((daemon_t*)daemon)->m_buildId);
        header->set_buildType(((daemon_t*)daemon)->m_buildType);
        header->set_branchId(((daemon_t*)daemon)->m_branchId);
        *header->get_product() = ((daemon_t*)daemon)->m_product;
        header->set_bufsz(0);

        unsigned int g = server::get()->config().get<unsigned int>("crypt", "db_g");
        const ST::string& n = server::get()->config().get<const ST::string&>("crypt", "db_n");
        const ST::string& x = server::get()->config().get<const ST::string&>("crypt", "db_x");
        db_client_connect(daemon->m_db, (sockaddr*)&addr, header, db_client_header_size(),
                          g, n, x, (client_connect_cb)db_connected);
    }
}

void fus::db_trans_daemon_free(fus::db_trans_daemon_t* daemon)
{
    secure_daemon_free((secure_daemon_t*)daemon);
}

void fus::db_trans_daemon_shutdown(fus::db_trans_daemon_t* daemon)
{
    secure_daemon_shutdown((secure_daemon_t*)daemon);

    // OK to free the database connection now.
    tcp_stream_free_on_close((tcp_stream_t*)daemon->m_db, true);
    tcp_stream_shutdown((tcp_stream_t*)daemon->m_db);
}
