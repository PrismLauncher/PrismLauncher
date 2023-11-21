// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 *
 */
#include "PluginContribution.h"

#include <type_traits>

static ExtentionPointRegistry* self = nullptr;

ExtentionPointRegistry& ExtentionPointRegistry::instance() noexcept
{
    if (!self) {
        self = new ExtentionPointRegistry();
    }
    return *self;
}

bool ExtentionPointRegistry::isKnown(const QString& kind) const
{
    return m_factories.find(kind) != m_factories.end();
}

void ExtentionPointRegistry::withFactory(const QString& kind, std::function<void(ExtentionPointRegistry::Factory&)> action)
{
    auto iter = m_factories.find(kind);
    if (iter != m_factories.end()) {
        action(*iter);
    }
}

void ExtentionPointRegistry::registerExtensionpoint(QString name, ExtentionPointRegistry::Factory factory)
{
    // TODO: check for duplicates
    m_factories.insert(name, factory);
}

template<class T>
void ExtentionPointRegistry::registerExtensionpoint(QString name) {
    static_assert(std::is_base_of<PluginContribution, T>::value, "T must derive from PluginContribution");

    this->registerExtensionpoint(name, [] () { return new T(); });
}

#include "extension_points/CatPackExtension.h"

__attribute__((constructor))
static void __registerExtensionPoints() {
    ExtentionPointRegistry& registry = ExtentionPointRegistry::instance();
    #define REGISTER(name, clazz) registry.registerExtensionpoint<clazz>(#name);

    REGISTER(cat_packs, CatPackContribution);
}
