#pragma once
#include "QObjectPtr.h"
#include "tasks/Task.h"
#include "minecraft/mod/ModFolderModel.h"
#include "net/NetJob.h"
#include <QUrl>


class ModDownloadTask : public Task {
    Q_OBJECT
public:
    explicit ModDownloadTask(const QUrl sourceUrl, const QString filename, const std::shared_ptr<ModFolderModel> mods);

public slots:
    bool abort() override;
protected:
    //! Entry point for tasks.
    void executeTask() override;

private:
    QUrl m_sourceUrl;
    NetJob::Ptr m_filesNetJob;
    const std::shared_ptr<ModFolderModel> mods;
    const QString filename;

    void downloadProgressChanged(qint64 current, qint64 total);

    void downloadFailed(QString reason);

    void downloadSucceeded();
};



