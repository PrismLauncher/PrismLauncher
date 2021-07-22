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
    
    
    /// Some services require the access token to be sent as a Authentication HTTP header
    /// and refuse requests with the access token in the query.
    /// This function allows to use or ignore the access token in the query.
    /// The default value of `true` means that the query will contain the access token.
    /// By setting the value to false, the query will not contain the access token.
    /// See:
    /// https://tools.ietf.org/html/draft-ietf-oauth-v2-bearer-16#section-4.3
    /// https://tools.ietf.org/html/rfc6750#section-2.3
    
    void setAddAccessTokenInQuery(bool value);

    /// Some services require the access token to be sent as a Authentication HTTP header.
    /// This is the case for Twitch and Mixer.
    /// When the access token expires and is refreshed, O2Requestor::retry() needs to update the Authentication HTTP header.
    /// In order to do so, O2Requestor needs to know the format of the Authentication HTTP header.
    void setAccessTokenInAuthenticationHTTPHeaderFormat(const QString &value);

public slots:
    /// Make a GET request.
    /// @return Request ID or -1 if there are too many requests in the queue.
    int get(const QNetworkRequest &req, int timeout = 60*1000);

    /// Make a POST request.
    /// @return Request ID or -1 if there are too many requests in the queue.
    int post(const QNetworkRequest &req, const QByteArray &data, int timeout = 60*1000);
    int post(const QNetworkRequest &req, QHttpMultiPart* data, int timeout = 60*1000);

    /// Make a PUT request.
    /// @return Request ID or -1 if there are too many requests in the queue.
    int put(const QNetworkRequest &req, const QByteArray &data, int timeout = 60*1000);
    int put(const QNetworkRequest &req, QHttpMultiPart* data, int timeout = 60*1000);

    /// Make a HEAD request.
    /// @return Request ID or -1 if there are too many requests in the queue.
    int head(const QNetworkRequest &req, int timeout = 60*1000);

    /// Make a custom request.
    /// @return Request ID or -1 if there are too many requests in the queue.
    int customRequest(const QNetworkRequest &req, const QByteArray &verb, const QByteArray &data, int timeout = 60*1000);

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
    QHttpMultiPart* multipartData_;
    QNetworkReply *reply_;
    Status status_;
    int id_;
    QNetworkAccessManager::Operation operation_;
    QUrl url_;
    ReplyList timedReplies_;
    QNetworkReply::NetworkError error_;
    bool addAccessTokenInQuery_;
    QString accessTokenInAuthenticationHTTPHeaderFormat_;
    bool rawData_;
};

}
