#pragma once
#include "tasks/Task.h"
#include <QNetworkReply>
#include <QBuffer>
#include <memory>

class PasteUpload : public Task
{
    Q_OBJECT
public:
    PasteUpload(QWidget *window, QString text, QString url);
    virtual ~PasteUpload();

    QString pasteLink()
    {
        return m_pasteLink;
    }
protected:
    virtual void executeTask();

private:
    QWidget *m_window;
    QString m_pasteLink;
    QString m_uploadUrl;
    QByteArray m_text;
    std::shared_ptr<QNetworkReply> m_reply;
public
slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};
