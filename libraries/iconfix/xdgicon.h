/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#pragma once

#include <QtGui/QIcon>
#include <QString>
#include <QStringList>

#include "multimc_iconfix_export.h"

class MULTIMC_ICONFIX_EXPORT XdgIcon
{
public:
    static QIcon fromTheme(const QString &iconName, const QIcon &fallback = QIcon());
    static QIcon fromTheme(const QStringList &iconNames, const QIcon &fallback = QIcon());

    static QString themeName();
    static void setThemeName(const QString &themeName);

protected:
    explicit XdgIcon();
    virtual ~XdgIcon();
};
