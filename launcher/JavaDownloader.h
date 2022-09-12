#pragma once

#include <QString>
#include "net/NetJob.h"
#include "tasks/Task.h"

class JavaDownloader : public Task {
    Q_OBJECT
   public:
    /*Downloads the java to the runtimes folder*/
    explicit JavaDownloader(bool isLegacy, const QString& OS) : m_isLegacy(isLegacy), m_OS(OS) {}

    void executeTask() override;
    [[nodiscard]] bool canAbort() const override { return true; }
    static void showPrompts(QWidget* parent = nullptr);

   private:
    bool m_isLegacy;
    const QString& m_OS;
    static void abortNetJob(NetJob* elementDownload);
};
