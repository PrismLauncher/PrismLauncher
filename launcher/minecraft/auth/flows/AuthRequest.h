#pragma once
#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QByteArray>
#include <QHttpMultiPart>

#include "katabasis/Reply.h"

/// Makes authentication requests.
class AuthRequest: public QObject {
    Q_OBJECT

public:
    explicit AuthRequest(QObject *parent = 0);
    ~AuthRequest();

public slots:
    void get(const QNetworkRequest &req, int timeout = 60*1000);
    void post(const QNetworkRequest &req, const QByteArray &data, int timeout = 60*1000);


signals:

    /// Emitted when a request has been completed or failed.
    void finished(QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair> headers);

    /// Emitted when an upload has progressed.
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected slots:

    /// Handle request finished.
    void onRequestFinished();

    /// Handle request error.
    void onRequestError(QNetworkReply::NetworkError error);

    /// Handle ssl errors.
    void onSslErrors(QList<QSslError> errors);

    /// Finish the request, emit finished() signal.
    void finish();

    /// Handle upload progress.
    void onUploadProgress(qint64 uploaded, qint64 total);

protected:
    void setup(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, const QByteArray &verb = QByteArray());

    enum Status {
        Idle, Requesting, ReRequesting
    };

    QNetworkRequest request_;
    QByteArray data_;
    QNetworkReply *reply_;
    Status status_;
    QNetworkAccessManager::Operation operation_;
    QUrl url_;
    Katabasis::ReplyList timedReplies_;
    QNetworkReply::NetworkError error_;
};
