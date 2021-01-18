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

#include <memory>
#include <QString>
#include <QMetaType>

/*!
 * An abstract base class for versions.
 */
class BaseVersion
{
public:
    virtual ~BaseVersion() {}
    /*!
     * A string used to identify this version in config files.
     * This should be unique within the version list or shenanigans will occur.
     */
    virtual QString descriptor() = 0;

    /*!
     * The name of this version as it is displayed to the user.
     * For example: "1.5.1"
     */
    virtual QString name() = 0;

    /*!
     * This should return a string that describes
     * the kind of version this is (Stable, Beta, Snapshot, whatever)
     */
    virtual QString typeString() const = 0;

    virtual bool operator<(BaseVersion &a)
    {
        return name() < a.name();
    };
    virtual bool operator>(BaseVersion &a)
    {
        return name() > a.name();
    };
};

typedef std::shared_ptr<BaseVersion> BaseVersionPtr;

Q_DECLARE_METATYPE(BaseVersionPtr)
