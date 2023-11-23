// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QMap>
#include <QWidget>

namespace Ui {
class EnvironmentVariables;
}

class EnvironmentVariables : public QWidget {
    Q_OBJECT

   public:
    explicit EnvironmentVariables(QWidget* state = nullptr);
    ~EnvironmentVariables() override;
    void initialize(bool instance, bool override, const QMap<QString, QVariant>& value);
    bool eventFilter(QObject* watched, QEvent* event) override;

    void retranslate();
    bool override() const;
    QMap<QString, QVariant> value() const;

   private:
    Ui::EnvironmentVariables* ui;
};
