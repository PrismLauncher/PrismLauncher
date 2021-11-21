#pragma once

#include <QObject>

#include "net/NetJob.h"
#include "net/Download.h"

class NotificationChecker : public QObject
{
    Q_OBJECT

public:
    explicit NotificationChecker(QObject *parent = 0);

    void setNotificationsUrl(const QUrl &notificationsUrl);
    void setApplicationPlatform(QString platform);
    void setApplicationChannel(QString channel);
    void setApplicationFullVersion(QString version);

    struct NotificationEntry
    {
        int id;
        QString message;
        enum
        {
            Critical,
            Warning,
            Information
        } type;
        QString channel;
        QString platform;
        QString from;
        QString to;
    };

    QList<NotificationEntry> notificationEntries() const;

public
slots:
    void checkForNotifications();

private
slots:
    void downloadSucceeded(int);

signals:
    void notificationCheckFinished();

private:
    bool entryApplies(const NotificationEntry &entry) const;

private:
    QList<NotificationEntry> m_entries;
    QUrl m_notificationsUrl;
    NetJob::Ptr m_checkJob;
    Net::Download::Ptr m_download;

    QString m_appVersionChannel;
    QString m_appPlatform;
    QString m_appFullVersion;
};
