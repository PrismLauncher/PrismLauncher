#pragma once
#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QByteArray>
#include <QHttpMultiPart>

#include "Reply.h"

namespace Katabasis {

class OAuth2;

/// Makes authenticated requests.
class Requestor: public QObject {
    Q_OBJECT

public:
    explicit Requestor(QNetworkAccessManager *manager, OAuth2 *authenticator, QObject *parent = 0);
    ~Requestor();

public slots:
    int get(const QNetworkRequest &req, int timeout = 60*1000);
    int post(const QNetworkRequest &req, const QByteArray &data, int timeout = 60*1000);


signals:

    /// Emitted when a request has been completed or failed.
    void finished(int id, QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair> headers);

    /// Emitted when an upload has progressed.
    void uploadProgress(int id, qint64 bytesSent, qint64 bytesTotal);

protected slots:
    /// Handle refresh completion.
    void onRefreshFinished(QNetworkReply::NetworkError error);

    /// Handle request finished.
    void onRequestFinished();

    /// Handle request error.
    void onRequestError(QNetworkReply::NetworkError error);

    /// Handle ssl errors.
    void onSslErrors(QList<QSslError> errors);

    /// Re-try request (after successful token refresh).
    void retry();

    /// Finish the request, emit finished() signal.
    void finish();

    /// Handle upload progress.
    void onUploadProgress(qint64 uploaded, qint64 total);

protected:
    int setup(const QNetworkRequest &request, QNetworkAccessManager::Operation operation, const QByteArray &verb = QByteArray());

    enum Status {
        Idle, Requesting, ReRequesting
    };

    QNetworkAccessManager *manager_;
    OAuth2 *authenticator_;
    QNetworkRequest request_;
    QByteArray data_;
    QNetworkReply *reply_;
    Status status_;
    int id_;
    QNetworkAccessManager::Operation operation_;
    QUrl url_;
    ReplyList timedReplies_;
    QNetworkReply::NetworkError error_;
};

}
