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

#include "core/endian.h"
#include "core/errors.h"
#include "hash.h"
#include <openssl/evp.h>

// =================================================================================

fus::hash::hash(hash_type type)
    : m_ctx(EVP_MD_CTX_new()), m_type(type)
{
}

fus::hash::~hash()
{
    EVP_MD_CTX_free(m_ctx);
}

// =================================================================================

static inline const EVP_MD* md_type(fus::hash_type type)
{
    switch (type) {
    case fus::hash_type::e_sha0:
        // This will probably not work-- SHA0 is a broken digest and has been removed from
        // OpenSSL. Hence why we must look it up by string name. It's really only here for
        // completeness. It should not be used on a production shard.
        return EVP_get_digestbyname("sha");
    case fus::hash_type::e_sha1:
        return EVP_sha1();
    case fus::hash_type::e_sha2:
        return EVP_sha256();
    default:
        return nullptr;
    }
}

size_t fus::hash::digestsz() const
{
    return EVP_MD_size(md_type(m_type));
}

void fus::hash::hash_account(const ST::string& account, const ST::string& pass, void* hashbuf, size_t hashbufsz)
{
    EVP_DigestInit(m_ctx, md_type(m_type));

    switch (m_type) {
    case hash_type::e_sha0:
        {
            ST::string acctlower = account.to_lower();

            ST::string_stream ss;
            ss.append(pass.c_str(), pass.size() - 1);
            ss.append_char('\0');
            ss.append(acctlower.c_str(), acctlower.size() - 1);
            ss.append_char('\0');
            ST::utf16_buffer buf = ss.to_string().to_utf16();
            EVP_DigestUpdate(m_ctx, buf.data(), buf.size() * sizeof(char16_t));
        }
        break;

    case hash_type::e_sha1:
        EVP_DigestUpdate(m_ctx, pass.c_str(), pass.size());
        break;

    case hash_type::e_sha2:
        {
            ST::string acctlower = account.to_lower();
            EVP_DigestUpdate(m_ctx, pass.c_str(), pass.size());
            EVP_DigestUpdate(m_ctx, acctlower.c_str(), acctlower.size());
        }
        break;
    }

    EVP_DigestFinal(m_ctx, (unsigned char*)hashbuf, (unsigned int*)&hashbufsz);
}

void fus::hash::hash_login(const void* acctHash, size_t acctHashsz, uint32_t cliChallenge,
                           uint32_t srvChallenge, void* hashbuf, size_t hashbufsz)
{
    FUS_ASSERTD(acctHashsz >= hashbufsz);
    if (m_type == hash_type::e_sha1) {
        memcpy(hashbuf, acctHash, hashbufsz);
    } else {
        EVP_DigestInit(m_ctx, md_type(m_type));
        EVP_DigestUpdate(m_ctx, &cliChallenge, sizeof(cliChallenge));
        EVP_DigestUpdate(m_ctx, &srvChallenge, sizeof(srvChallenge));
        EVP_DigestUpdate(m_ctx, acctHash, hashbufsz); // acctHashsz might be larger than the digest length
        EVP_DigestFinal(m_ctx, (unsigned char*)hashbuf, (unsigned int*)&hashbufsz);
    }
}
