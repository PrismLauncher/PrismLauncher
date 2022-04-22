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
#include <QString>
#include <QStringList>

#include "QObjectPtr.h"

class Task : public QObject {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Task>;

    enum class State { Inactive, Running, Succeeded, Failed, AbortedByUser };

   public:
    explicit Task(QObject* parent = 0);
    virtual ~Task() = default;

    bool isRunning() const;
    bool isFinished() const;
    bool wasSuccessful() const;

    /*! 
     * MultiStep tasks are combinations of multiple tasks into a single logical task. 
     * The main usage of this is in SequencialTask.
     */
    virtual auto isMultiStep() const -> bool { return false; }

    /*!
     * Returns the string that was passed to emitFailed as the error message when the task failed.
     * If the task hasn't failed, returns an empty string.
     */
    QString failReason() const;

    virtual QStringList warnings() const;

    virtual bool canAbort() const { return false; }

    auto getState() const -> State { return m_state; }

    QString getStatus() { return m_status; }
    virtual auto getStepStatus() const -> QString { return m_status; }

    qint64 getProgress() { return m_progress; }
    qint64 getTotalProgress() { return m_progressTotal; }
    virtual auto getStepProgress() const -> qint64 { return 0; }
    virtual auto getStepTotalProgress() const -> qint64 { return 100; }

   protected:
    void logWarning(const QString& line);

   private:
    QString describe();

   signals:
    void started();
    virtual void progress(qint64 current, qint64 total);
    void finished();
    void succeeded();
    void failed(QString reason);
    void status(QString status);

   public slots:
    virtual void start();
    virtual bool abort() { return false; };

   protected:
    virtual void executeTask() = 0;

   protected slots:
    virtual void emitSucceeded();
    virtual void emitAborted();
    virtual void emitFailed(QString reason);

   public slots:
    void setStatus(const QString& status);
    void setProgress(qint64 current, qint64 total);

   protected:
    State m_state = State::Inactive;
    QStringList m_Warnings;
    QString m_failReason = "";
    QString m_status;
    int m_progress = 0;
    int m_progressTotal = 100;
};
