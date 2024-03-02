// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 初夏同学 <2411829240@qq.com>
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

#include "Setting.h"

/*!
 * \brief A setting used for command line. It cannot be modified or saved.
 */
class ConstantSetting : public Setting {
    Q_OBJECT
   public:
    explicit ConstantSetting(std::shared_ptr<Setting> origin, QVariant defVal = QVariant());

    virtual QVariant get() const;
    virtual void set(QVariant value);
    virtual void reset();

   protected:
    std::shared_ptr<Setting> m_other;
};
