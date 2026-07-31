// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include <QWidget>

#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class DevelopingModPage;
}

class DevelopingModPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit DevelopingModPage(MinecraftInstance* inst, QWidget* parent = nullptr);
    virtual ~DevelopingModPage();

    QString displayName() const override { return tr("Developing Mod"); }
    QIcon icon() const override
    {
        auto icon = QIcon::fromTheme("loadermods");
        if (icon.isNull())
            icon = QIcon::fromTheme("folder");
        return icon;
    }
    QString id() const override { return "developing-mod"; }
    bool apply() override;
    QString helpPage() const override { return "Developing-Mod"; }
    void retranslate() override;
    void openedImpl() override;

   private:
    enum class WatchKind { Folder = 0, Jar = 1 };

   private slots:
    void enableToggled(bool checked);
    void addFolder();
    void addJar();
    void removeSelected();
    void syncNowClicked();
    void updateStatusLabel(const QString& status);

   private:
    void loadFromSettings();
    void setControlsEnabled(bool enabled);
    void saveWatchTargets();
    void saveIgnorePatterns();
    void addWatchItem(WatchKind kind, const QString& path);
    bool containsPath(const QString& path) const;
    QStringList pathsOfKind(WatchKind kind) const;
    static QString displayText(WatchKind kind, const QString& path);

    Ui::DevelopingModPage* ui;
    MinecraftInstance* m_inst;
};
