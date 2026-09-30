// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QPair>
#include <QString>

namespace api {

/**
 * Settings that end up on a command line (Java executable, JVM arguments, custom commands) let whoever sets them
 * run arbitrary programs. The web UI may request such changes, but they are only applied after the user confirms
 * them in a native dialog that page content cannot draw over or click.
 */
inline bool confirmSensitiveChange(const QString& scope, const QList<QPair<QString, QString>>& changes)
{
    QString text = QObject::tr("The launcher UI wants to change settings that control which programs are run (%1):").arg(scope);
    QString details;
    for (const auto& [key, value] : changes) {
        details += QStringLiteral("%1 = %2\n").arg(key, value.isEmpty() ? QObject::tr("(empty)") : value);
    }
    QMessageBox box(QMessageBox::Warning, QObject::tr("Confirm settings change"), text, QMessageBox::Yes | QMessageBox::No);
    box.setInformativeText(QObject::tr("Only continue if you made this change yourself."));
    box.setDetailedText(details);
    box.setDefaultButton(QMessageBox::No);
    return box.exec() == QMessageBox::Yes;
}

}  // namespace api
