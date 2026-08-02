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

#include <launch/LaunchStep.h>
#include <memory>

class ScanModFolders : public LaunchStep {
    Q_OBJECT
   public:
    explicit ScanModFolders(LaunchTask* parent) : LaunchStep(parent) {};
    virtual ~ScanModFolders() {};

    virtual void executeTask() override;
    virtual bool canAbort() const override { return false; }
   private slots:
    void coreModsDone();
    void modsDone();
    void nilModsDone();
    // Re-runs checkDone() on every ModFolderModel::parseFinished so the launch
    // apply happens as soon as the last per-mod parse task completes.
    void onParseFinished();

   private:
    void checkDone();
    // Reads runtime-profile selection from instance settings and applies the
    // union of enabled mod IDs to each folder model. Called once, synchronously,
    // after all three folder scans are complete AND no per-mod parse tasks are
    // pending (see checkDone()). Does nothing if no runtime selection has been
    // configured (preserving the existing launch behaviour).
    void applyRuntimeProfiles();

   private:  // DATA
    bool m_modsDone = false;
    bool m_nilModsDone = false;
    bool m_coreModsDone = false;
    // Guards the runningStatusChanged connection in applyRuntimeProfiles()
    // so it's registered exactly once, not once per launch.
    bool m_restoreListenerConnected = false;
    // True once applyRuntimeProfiles() + emitSucceeded() have run. checkDone()
    // is reachable again via parseFinished, so this prevents a double apply.
    bool m_applyDone = false;
};
