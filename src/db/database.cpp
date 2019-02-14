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

#include "core/config_parser.h"
#include "core/errors.h"
#include <cstring>
#include "database.h"
#include "db_sqlite3.h"
#include "io/log_file.h"

 // =================================================================================

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
