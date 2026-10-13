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

#include <QDialog>

#include "meta/Version.h"
#include "meta/VersionList.h"
#include "ui/pages/BasePageProvider.h"

class MinecraftInstance;
class PageContainer;
class PackProfile;
class QDialogButtonBox;

class InstallLoaderDialog final : public QDialog, protected BasePageProvider {
    Q_OBJECT

   public:
    explicit InstallLoaderDialog(QString gameVersion, PackProfile* profile, QWidget* parent = nullptr);

    struct Result {
        Meta::VersionList::Ptr list;
        Meta::Version::Ptr version;
    };

    static std::optional<Result> choose(const QString& gameVersion, QWidget* parent = nullptr);

    static bool chooseAndInstall(PackProfile* profile, QWidget* parent = nullptr);

    QString dialogTitle() override;

    std::optional<Result> getResult() const;

   protected:
    QList<BasePage*> getPages() override;

   private:
    void validate(BasePage* page);

    QString m_gameVersion;
    PackProfile* m_profile;
    PageContainer* m_container;
    QDialogButtonBox* m_buttons;
};
