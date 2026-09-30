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

#include <QObject>
#include <QSet>
#include <QTimer>

namespace api {

class ApiRouter;

/**
 * `settings.get` / `settings.set` over an explicit allow-list of global launcher settings
 * (`LauncherSettings` in frontend/src/types/settings.ts). Unknown keys are rejected; keys that
 * control executed programs require native confirmation (see SensitiveChange.h).
 * External changes (e.g. from the Qt settings dialog) are pushed as `settings.changed`.
 */
class SettingsApi : public QObject {
    Q_OBJECT
   public:
    explicit SettingsApi(ApiRouter* router, QObject* parent = nullptr);

   private:
    ApiRouter* m_router;
    QTimer m_changedTimer;
    QSet<QString> m_changedKeys;
};

}  // namespace api
