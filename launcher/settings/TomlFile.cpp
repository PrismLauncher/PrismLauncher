// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "TomlFile.h"

#include "FileSystem.h"
#include "settings/INIFile.h"

#include <QVariant>

#include <fstream>
#include <string>

bool TomlFile::loadFile(QString fileName)
{
#if TOML_EXCEPTIONS
    try {
        m_data = toml::parse_file(fileName.toStdString());
    } catch (const toml::parse_error& err) {
        qWarning() << QString("Could not open file %1!").arg(fileName);
        qWarning() << "Reason: " << QString(err.what());
        return migrate(fileName);
    }
#else
    auto result = toml::parse_file(fileName.toStdString());
    if (!result) {
        qWarning() << QString("Could not open file %1!").arg(fileName);
        qWarning() << "Reason: " << result.error().description();
        return migrate(fileName);
    }
    m_data = result.table();
#endif

    /*migration*/
    if (!m_data.empty() && (!m_data.contains("ConfigVersion") || !m_data.get("ConfigVersion")->is_string() ||
                            m_data.get("ConfigVersion")->value<std::string>().value() != "1.3")) {
        m_loaded = migrate(fileName);
    } else {
        m_loaded = true;
    }
    return m_loaded;
}

bool TomlFile::loadFile(QByteArray data)
{
#if TOML_EXCEPTIONS
    try {
        m_data = toml::parse(data.toStdString());
    } catch (const toml::parse_error& err) {
        return false;
    }
#else
    auto result = toml::parse(data.toStdString());
    if (!result) {
        return false;
    }
    m_data = result.table();
#endif
    return true;
}

toml::table merge_tables(const toml::table& table1, const toml::table& table2)
{
    toml::table merged = table1;

    for (const auto& [key, value] : table2) {
        if (merged.contains(key)) {
            if (merged[key].is_table() && value.is_table()) {
                merged.insert_or_assign(key, merge_tables(*merged[key].as_table(), *value.as_table()));
            } else {
                merged.insert_or_assign(key, value);
            }
        } else {
            merged.insert_or_assign(key, value);
        }
    }

    return merged;
}

bool TomlFile::saveFile(QString fileName)
{
    if (!m_loaded) {
        auto tmp = m_data;
        m_data = {};
        loadFile(fileName);
        m_data = merge_tables(m_data, tmp);
    }
    if (!contains("ConfigVersion"))
        set("ConfigVersion", "1.3");
    std::ofstream outFile;
    outFile.open(fileName.toStdString());
    if (!outFile.is_open()) {
        qCritical() << QString("Could not open file %1!").arg(fileName);
        return false;
    }
    outFile << m_data;
    outFile.flush();
    outFile.close();
    return true;
}

QVariant TomlFile::get(QString key, QVariant def) const
{
    auto stdKey = key.toStdString();
    if (!m_data.contains(stdKey))
        return def;
    auto node = m_data.get(stdKey);
    switch (node->type()) {
        case toml::node_type::none:
            return def;
        case toml::node_type::table:
            if (auto table = node->as_table();  // we may want in the future to support multiple QT types
                table->contains("type") && table->contains("value") && table->get("type")->value<std::string>() == "QByteArray")
                return QByteArray::fromBase64(QByteArray::fromStdString(table->get("value")->value<std::string>().value()));
            return QVariant::fromValue(node->as_table());
        case toml::node_type::array:
            return QVariant::fromValue(node->as_array());
        case toml::node_type::string: {
            auto value = node->value<std::string>().value();
            return QString::fromStdString(value);
        }
        case toml::node_type::integer: {
            auto value = node->value<int>().value();
            return value;
        }
        case toml::node_type::floating_point: {
            auto value = node->value<float>().value();
            return value;
        }
        case toml::node_type::boolean: {
            auto value = node->value<bool>().value();
            return value;
        }
        case toml::node_type::date:
        /* fallthrough */
        case toml::node_type::time:
        /* fallthrough */
        case toml::node_type::date_time:
        /* fallthrough */
        default:
            return {};
    }
}

bool isEmptyNode(const toml::node& node)
{
    return (node.is_string() && node.as_string()->get().empty()) || (node.is_integer() && node.as_integer()->get() == 0) ||
           (node.is_floating_point() && node.as_floating_point()->get() == 0.0) ||
           (node.is_boolean() && node.as_boolean()->get() == false) || (node.is_array() && node.as_array()->empty()) ||
           (node.is_table() && node.as_table()->empty());
}

// Function to filter out empty fields from a TOML table
void filterEmptyFields(toml::table& table)
{
    for (auto it = table.begin(); it != table.end();) {
        auto& value = it->second;
        if (value.is_table()) {
            filterEmptyFields(*value.as_table());
        }
        if (isEmptyNode(value)) {
            it = table.erase(it);
        } else {
            ++it;
        }
    }
}

void TomlFile::set(QString key, QVariant val)
{
    auto stdKey = key.toStdString();

    switch (val.type()) {
        case QVariant::Bool:
            m_data.insert_or_assign(stdKey, val.toBool());
            break;
        case QVariant::Int:
            m_data.insert_or_assign(stdKey, val.toInt());
            break;
        case QVariant::UInt:
            m_data.insert_or_assign(stdKey, val.toUInt());
            break;
        case QVariant::LongLong:
            m_data.insert_or_assign(stdKey, val.toLongLong());
            break;
        case QVariant::ULongLong:
            m_data.insert_or_assign(stdKey, val.toLongLong());
            break;
        case QVariant::Double:
            m_data.insert_or_assign(stdKey, val.toDouble());
            break;
        case QVariant::Char:
        /* fallthrough */
        case QVariant::Url:
        /* fallthrough */
        case QVariant::String:
            m_data.insert_or_assign(stdKey, val.toString().toStdString());
            break;
        case QVariant::StringList: {
            toml::array array;
            for (auto s : val.toStringList())
                array.push_back(s.toStdString());
            m_data.insert_or_assign(stdKey, array);
            break;
        }
        case QVariant::ByteArray:
            m_data.insert_or_assign(stdKey,
                                    toml::table{ { "type", "QByteArray" }, { "value", val.toByteArray().toBase64().toStdString() } });
            break;
        case QVariant::Invalid:
            break;
        // case QVariant::BitArray:
        // case QVariant::Date:
        // case QVariant::Time:
        // case QVariant::DateTime:
        // case QVariant::Map:
        // case QVariant::List:
        default:
            // minimize the content by removing empty fields
            if (val.canConvert<toml::table>()) {
                auto table = val.value<toml::table>();
                filterEmptyFields(table);
                m_data.insert_or_assign(stdKey, table);
            } else if (val.canConvert<toml::array>()) {
                auto array = val.value<toml::array>();
                if (array.is_array_of_tables()) {
                    for (auto& element : array) {
                        if (element.is_table()) {
                            filterEmptyFields(*element.as_table());
                        }
                    }
                }
                m_data.insert_or_assign(stdKey, array);
            }
            break;
    }
}

void TomlFile::remove(QString key)
{
    m_data.erase(key.toStdString());
}

bool TomlFile::contains(QString key) const
{
    return m_data.contains(key.toStdString());
}

QVariant TomlFile::operator[](const QString& key) const
{
    return get(key);
}

void collect_keys(const toml::node* node, QStringList& keys, const QString& prefix = "")
{
    if (node->is_table()) {
        const auto& table = node->as_table();
        for (const auto& [key, val] : *table) {
            auto keyStr = QString::fromStdString(std::string(key.str()));
            auto new_prefix = prefix.isEmpty() ? keyStr : prefix + "." + keyStr;
            keys.append(new_prefix);
            collect_keys(&val, keys, new_prefix);
        }
    } else if (node->is_array()) {
        const auto& array = node->as_array();
        for (size_t i = 0; i < array->size(); ++i) {
            auto new_prefix = prefix + "[" + QString::number(i) + "]";
            keys.append(new_prefix);
            collect_keys(array->get(i), keys, new_prefix);
        }
    }
}

QStringList TomlFile::keys()
{
    QStringList v;
    collect_keys(&m_data, v);
    return v;
}

bool TomlFile::migrate(QString fileName)
{
    m_data = {};
    INIFile f;
    if (!f.loadFile(fileName))
        return false;

    for (auto key : f.keys())
        set(key, f.get(key));

    set("ConfigVersion", "1.3");
    m_loaded = true;
    FS::move(fileName, fileName + ".old");
    return saveFile(fileName);
}