// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include <QDebug>
#include <QList>
#include <QString>
#include <QStringView>

// this implements the FlexVer
// https://git.sleeping.town/exa/FlexVer
class Version {
   public:
    Version(QString str) : m_string(std::move(str)) { parse(); }  // NOLINT(hicpp-explicit-conversions)
    Version() = default;

   private:
    struct Section {
        enum class Type : std::uint8_t { Null, Textual, Numeric, PreRelease };
        explicit Section(Type t = Type::Null, QString value = "") : t(t), value(std::move(value)) {}
        Type t;
        QString value;
        bool operator==(const Section& other) const = default;
        std::strong_ordering operator<=>(const Section& other) const;
    };

   private:
    void parse();

   public:
    QString toString() const { return m_string; }
    bool isEmpty() const { return m_string.isEmpty(); }

    friend QDebug operator<<(QDebug debug, const Version& v);

    bool operator==(const Version& other) const { return (*this <=> other) == std::strong_ordering::equal; }
    std::strong_ordering operator<=>(const Version& other) const;

   private:
    QString m_string;
    QList<Section> m_sections;
};