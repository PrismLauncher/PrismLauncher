// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "BaseVersion.h"
#include "JavaVersion.h"

struct JavaInstall : public BaseVersion {
    JavaInstall() {}
    JavaInstall(QString id, QString arch, QString path) : id(id), arch(arch), path(path) {}
    virtual QString descriptor() override { return id.toString(); }

    virtual QString name() override { return id.toString(); }

    virtual QString typeString() const override { return arch; }

    virtual bool operator<(BaseVersion& a) override;
    virtual bool operator>(BaseVersion& a) override;
    bool operator<(const JavaInstall& rhs);
    bool operator==(const JavaInstall& rhs);
    bool operator>(const JavaInstall& rhs);

    JavaVersion id;
    QString arch;
    QString path;
    bool recommended = false;
    bool is_64bit = false;
};

using JavaInstallPtr = std::shared_ptr<JavaInstall>;
