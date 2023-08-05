/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <memory>

#include "Setting.h"

/*!
 * \brief A setting that 'overrides another.' based on the value of a 'gate' setting
 * If 'gate' evaluates to true, the override stores and returns data
 * If 'gate' evaluates to false, the original does,
 */
class PassthroughSetting : public Setting {
    Q_OBJECT
   public:
    explicit PassthroughSetting(std::shared_ptr<Setting> overriden, std::shared_ptr<Setting> gate);

    virtual QVariant defValue() const;
    virtual QVariant get() const;
    virtual void set(QVariant value);
    virtual void reset();

   private:
    bool isOverriding() const;

   protected:
    std::shared_ptr<Setting> m_other;
    std::shared_ptr<Setting> m_gate;
};
