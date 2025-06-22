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

class CheckJava : public LaunchStep {
    Q_OBJECT
   public:
    explicit CheckJava(LaunchTask* parent) : LaunchStep(parent) {};
    virtual ~CheckJava() = default;

    virtual void executeTask();
    virtual bool canAbort() const { return false; }
   private slots:
    void checkJavaFinished(const JavaChecker::Result& result);

   private:
    void printJavaInfo(const QString& version, const QString& architecture, const QString& realArchitecture, const QString& vendor);
    void printSystemInfo(bool javaIsKnown, bool javaIs64bit);

   private:
    QString m_javaPath;
    QString m_javaSignature;
    JavaChecker::Ptr m_JavaChecker;
};
