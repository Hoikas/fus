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

#include "constants.h"
#include "core/config_parser.h"
#include "core/endian.h"
#include "core/errors.h"
#include <cstring>
#include "database.h"
#include "db_sqlite3.h"
#include "io/log_file.h"
#include <openssl/evp.h>

 // =================================================================================

fus::database::database()
    : m_log(), m_hashCtx(EVP_MD_CTX_new())
{

}

fus::database::~database()
{
    EVP_MD_CTX_free(m_hashCtx);
}

fus::database* fus::database::init(const fus::config_parser& config, fus::log_file& log)
{
    database* db = nullptr;
    ST::string engine = config.get<const ST::string&>("db", "engine").to_lower();
    if (engine == ST_LITERAL("sqlite") || engine == ST_LITERAL("sqlite3")) {
        db = new db_sqlite3;
    } else {
        log.write_error("Unknown database engine requested: '{}'", engine);
        return nullptr;
    }

    db->m_log = &log;
    if (!db->open(config)) {
        delete db;
        return nullptr;
    }
    return db;
}

// =================================================================================

const EVP_MD* fus::database::digest(fus::hash_type hashType) const
{
    // Presently, only SHA-1 is supported.
    FUS_ASSERTD(hashType == fus::hash_type::e_sha1);
    return EVP_sha1();
}

size_t fus::database::digestsz(fus::hash_type hashType) const
{
    return EVP_MD_size(digest(hashType));
}

// =================================================================================

bool fus::database::compare_hash(uint32_t cliChallenge, uint32_t srvChallenge, fus::hash_type hashType,
                                 const void* dbHashBuf, size_t dbHashBufsz,
                                 const void* hashBuf, size_t hashBufsz)
{
    FUS_ASSERTD(dbHashBufsz == digestsz(hashType));
    FUS_ASSERTD(hashBufsz == digestsz(hashType));

    switch (hashType) {
    case hash_type::e_sha0:
        {
            uint32_t cc = FUS_LE32(cliChallenge);
            uint32_t sc = FUS_LE32(srvChallenge);

            EVP_DigestInit_ex(m_hashCtx, digest(hashType), nullptr);
            EVP_DigestUpdate(m_hashCtx, &cc, sizeof(uint32_t));
            EVP_DigestUpdate(m_hashCtx, &sc, sizeof(uint32_t));
            EVP_DigestUpdate(m_hashCtx, dbHashBuf, dbHashBufsz);

            size_t resultHashBufsz = EVP_MD_CTX_size(m_hashCtx);
            void* resultHashBuf = alloca(resultHashBufsz);
            unsigned int nWrite;
            EVP_DigestFinal_ex(m_hashCtx, (unsigned char*)resultHashBuf, &nWrite);

            return memcmp(resultHashBuf, hashBuf, resultHashBufsz) == 0;
        }
    case hash_type::e_sha1:
        return memcmp(dbHashBuf, hashBuf, dbHashBufsz) == 0;
    }
}

void fus::database::hash_account(const std::u16string_view& name, const std::string_view& pass,
                                 fus::hash_type hashType, void* outBuf, size_t outBufsz)
{
    FUS_ASSERTD(outBuf);
    FUS_ASSERTD(outBufsz == digestsz(hashType));

    EVP_DigestInit_ex(m_hashCtx, digest(hashType), nullptr);
    switch (hashType) {
    case hash_type::e_sha0:
        {
            ST::string name_lower = ST::string(name).to_lower();

            // Reference implementation of buggy, domain based hashes.
            ST::string_stream ss;
            ss.append(pass.data(), pass.size() - 1);
            ss.append_char('\0');
            ss.append(name_lower.c_str(), name_lower.size() - 1);
            ss.append_char('\0');
            ST::utf16_buffer tempbuf = ss.to_string().to_utf16();
            EVP_DigestUpdate(m_hashCtx, tempbuf.data(), tempbuf.size() * sizeof(char16_t));
        }
        break;
    case hash_type::e_sha1:
        // Password only
        EVP_DigestUpdate(m_hashCtx, pass.data(), pass.size());
        break;
    default:
        FUS_ASSERTR(0);
    }

    unsigned int nWrite;
    EVP_DigestFinal_ex(m_hashCtx, (unsigned char*)outBuf, &nWrite);
}

// =================================================================================

void fus::database::extract_hash(uint32_t& flags, hash_type& hashType) const
{
    if (flags & acct_flags::e_acctHashSha0)
        hashType = hash_type::e_sha0;
    else if (flags & acct_flags::e_acctHashSha1)
        hashType = hash_type::e_sha1;
    else if (hashType != hash_type::e_unspecified)
        stuff_hash(flags, hashType);
}

void fus::database::stuff_hash(uint32_t& flags, fus::hash_type hashType) const
{
    flags &= ~acct_flags::e_acctHashMask;
    switch (hashType) {
    case hash_type::e_sha0:
        flags |= acct_flags::e_acctHashSha0;
        break;
    case hash_type::e_sha1:
        flags |= acct_flags::e_acctHashSha1;
        break;
    default:
        FUS_ASSERTR(0);
        break;
    }
}
