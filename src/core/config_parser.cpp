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

#include "config_parser.h"
#include "errors.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

// ============================================================================
template<>
bool fus::config_parser::get<bool>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_boolean);
    return item_it->second.m_value.to_bool();
}

template<>
float fus::config_parser::get<float>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_float);
    return item_it->second.m_value.to_float();
}

template<>
int fus::config_parser::get<int>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_integer);
    return item_it->second.m_value.to_int();
}

template<>
const char* fus::config_parser::get<const char*>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_string);
    return item_it->second.m_value.c_str();
}

template<>
ST::string fus::config_parser::get<ST::string>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_string);
    return item_it->second.m_value;
}

template<>
unsigned int fus::config_parser::get<unsigned int>(const ST::string& section, const ST::string& key) const
{
    auto section_it = m_config.find(section);
    FUS_ASSERTD(section_it != m_config.end());
    auto item_it = section_it->second.find(key);
    FUS_ASSERTD(item_it != section_it->second.end());
    FUS_ASSERTD(item_it->second.m_def->m_type == config_item::value_type::e_integer);
    return item_it->second.m_value.to_uint();
}

// ============================================================================
bool fus::config_parser::read(const std::filesystem::path& filename)
{
    if (!std::filesystem::exists(filename))
        return false;

    std::ifstream stream;
    stream.open(filename, std::ios_base::in);

    std::regex section_test("\\[(.*?)\\]");
    std::regex value_test("(\\w+)\\s*=\\s*([^\\+]+(?!\\+{3}))");

    size_t lineno = 0;
    configmap_t::iterator section = m_config.end();
    for (std::string buf; std::getline(stream, buf); ++lineno) {
        ST::string line = ST::string::from_std_string(buf).trim();
        if (line.empty())
            continue;
        if (line.starts_with(";") || line.starts_with("#") || line.starts_with("//"))
            continue;

        std::cmatch match;
        if (std::regex_search(line.c_str(), match, section_test)) {
            ST::string name = ST::string::from_std_string(match[1].str());
            section = m_config.find(name);
            if (section == m_config.end()) {
                std::cerr << "CONFIG: (Line: " << lineno << ") Found invalid section '" << name.c_str();
                std::cerr << "', ignoring." << std::endl;
            }
        } else if (std::regex_search(line.c_str(), match, value_test)) {
            ST::string key = ST::string::from_std_string(match[1].str());
            ST::string value = ST::string::from_std_string(match[2].str());

            if (section == m_config.end()) {
                std::cerr << "CONFIG: (Line: " << lineno << ") Found config item '" << key.c_str();
                std::cerr << "' outside of any section, ignoring." << std::endl;
                continue;
            }

            auto item = section->second.find(key);
            if (item != section->second.end()) {
                item->second.m_value = value;
            } else {
                std::cerr << "CONFIG: (Line: " << lineno << ") Found unknown config item '" << section->first.c_str();
                std::cerr << "." << key.c_str() << "' -- is it in the right section?" << std::endl;
            }
        } else {
            std::cerr << "CONFIG: (Line: " << lineno << ") WTF?!?!?!" << std::endl;
        }
    }
    return true;
}

bool fus::config_parser::write(const std::filesystem::path& filename)
{
    std::ofstream stream;
    stream.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!stream.is_open())
        return false;
    write(stream);
    return true;
}

void fus::config_parser::write(std::ostream& stream)
{
    for (auto section : m_config) {
        stream << "[" << section.first.c_str() << "]" << std::endl;
        for (auto item = section.second.begin(); item != section.second.end(); ++item) {
            std::vector<ST::string> comments = item->second.m_def->m_description.tokenize("\r\n");
            if (!comments.empty() && item != section.second.begin())
                stream << std::endl;
            for (auto comment : comments)
                stream << "; " << comment.c_str() << std::endl;
            stream << item->first.c_str() << " = " << item->second.m_value.c_str() << std::endl;
        }
        stream << std::endl;
    }
}
