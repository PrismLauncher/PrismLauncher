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

#include <LoggedProcess.h>
#include <java/JavaChecker.h>
#include <launch/LaunchStep.h>

/*
 * FIXME: maybe do not export
 */

class TextPrint : public LaunchStep {
    Q_OBJECT
   public:
    explicit TextPrint(LaunchTask* parent, const QStringList& lines, MessageLevel::Enum level);
    explicit TextPrint(LaunchTask* parent, const QString& line, MessageLevel::Enum level);
    virtual ~TextPrint() {};

    virtual void executeTask();
    virtual bool canAbort() const;
    virtual bool abort();

   private:
    QStringList m_lines;
    MessageLevel::Enum m_level;
};
