// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "Resource.h"

/* Info:
 * Currently For Optifine / Iris shader packs,
 * could be expanded to support others should they exist?
 *
 * This class and enum are mostly here as placeholders for validating
 * that a shaderpack exists and is in the right format,
 * namely that they contain a folder named 'shaders'.
 *
 * In the technical sense it would be possible to parse files like `shaders/shaders.properties`
 * to get information like the available profiles but this is not all that useful without more knowledge of the
 * shader mod used to be able to change settings.
 */

#include <QMutex>

enum class ShaderPackFormat { VALID, INVALID };

class ShaderPack : public Resource {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    [[nodiscard]] ShaderPackFormat packFormat() const { return m_pack_format; }

    ShaderPack(QObject* parent = nullptr) : Resource(parent) {}
    ShaderPack(QFileInfo file_info) : Resource(file_info) {}

    /** Thread-safe. */
    void setPackFormat(ShaderPackFormat new_format);

    bool valid() const override;

   protected:
    mutable QMutex m_data_lock;

    ShaderPackFormat m_pack_format = ShaderPackFormat::INVALID;
};
