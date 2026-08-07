// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QTreeWidgetItem>
#include <QWidget>

namespace Ui {
class LogAnonymization;
}

class LogAnonymization : public QWidget {
    Q_OBJECT

   public:
    struct Rule {
        QString regex;
        QString replace;
    };

    explicit LogAnonymization(QWidget* parent = nullptr);
    ~LogAnonymization() override;

    void initialize(const QList<Rule>& rules);
    bool eventFilter(QObject* watched, QEvent* event) override;

    void retranslate();
    QList<Rule> value() const;

   private:
    static void validateItem(QTreeWidgetItem* item);

    Ui::LogAnonymization* m_ui;
};
