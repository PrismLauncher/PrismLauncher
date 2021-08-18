#include <cassert>

#include <QDebug>
#include <QTimer>
#include <QBuffer>
#include <QUrlQuery>

#include "katabasis/Requestor.h"
#include "katabasis/OAuth2.h"
#include "katabasis/Globals.h"

namespace Katabasis {

Requestor::Requestor(QNetworkAccessManager *manager, OAuth2 *authenticator, QObject *parent): QObject(parent), reply_(NULL), status_(Idle), addAccessTokenInQuery_(true), rawData_(false) {
    manager_ = manager;
    authenticator_ = authenticator;
    if (authenticator) {
        timedReplies_.setIgnoreSslErrors(authenticator->ignoreSslErrors());
    }
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
    connect(authenticator, &OAuth2::refreshFinished, this, &Requestor::onRefreshFinished, Qt::QueuedConnection);
}

Requestor::~Requestor() {
}

void Requestor::setAddAccessTokenInQuery(bool value) {
    addAccessTokenInQuery_ = value;
}

void Requestor::setAccessTokenInAuthenticationHTTPHeaderFormat(const QString &value) {
    accessTokenInAuthenticationHTTPHeaderFormat_ = value;
}

int Requestor::get(const QNetworkRequest &req, int timeout/* = 60*1000*/) {
    if (-1 == setup(req, QNetworkAccessManager::GetOperation)) {
        return -1;
    }
    reply_ = manager_->get(request_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    return id_;
}

int Requestor::post(const QNetworkRequest &req, const QByteArray &data, int timeout/* = 60*1000*/) {
    if (-1 == setup(req, QNetworkAccessManager::PostOperation)) {
        return -1;
    }
    rawData_ = true;
    data_ = data;
    reply_ = manager_->post(request_, data_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    return id_;
}

int Requestor::post(const QNetworkRequest & req, QHttpMultiPart* data, int timeout/* = 60*1000*/)
{
    if (-1 == setup(req, QNetworkAccessManager::PostOperation)) {
        return -1;
    }
    rawData_ = false;
    multipartData_ = data;
    reply_ = manager_->post(request_, multipartData_);
    multipartData_->setParent(reply_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    return id_;
}

int Requestor::put(const QNetworkRequest &req, const QByteArray &data, int timeout/* = 60*1000*/) {
    if (-1 == setup(req, QNetworkAccessManager::PutOperation)) {
        return -1;
    }
    rawData_ = true;
    data_ = data;
    reply_ = manager_->put(request_, data_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    return id_;
}

int Requestor::put(const QNetworkRequest & req, QHttpMultiPart* data, int timeout/* = 60*1000*/)
{
    if (-1 == setup(req, QNetworkAccessManager::PutOperation)) {
        return -1;
    }
    rawData_ = false;
    multipartData_ = data;
    reply_ = manager_->put(request_, multipartData_);
    multipartData_->setParent(reply_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    return id_;
}

int Requestor::customRequest(const QNetworkRequest &req, const QByteArray &verb, const QByteArray &data, int timeout/* = 60*1000*/)
{
    (void)timeout;

    if (-1 == setup(req, QNetworkAccessManager::CustomOperation, verb)) {
        return -1;
    }
    data_ = data;
    QBuffer * buffer = new QBuffer;
    buffer->setData(data_);
    reply_ = manager_->sendCustomRequest(request_, verb, buffer);
    buffer->setParent(reply_);
    timedReplies_.add(new Reply(reply_));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    return id_;
}

int Requestor::head(const QNetworkRequest &req, int timeout/* = 60*1000*/)
{
    if (-1 == setup(req, QNetworkAccessManager::HeadOperation)) {
        return -1;
    }
    reply_ = manager_->head(request_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    return id_;
}

void Requestor::onRefreshFinished(QNetworkReply::NetworkError error) {
    if (status_ != Requesting) {
        qWarning() << "O2Requestor::onRefreshFinished: No pending request";
        return;
    }
    if (QNetworkReply::NoError == error) {
        QTimer::singleShot(100, this, &Requestor::retry);
    } else {
        error_ = error;
        QTimer::singleShot(10, this, &Requestor::finish);
    }
}

void Requestor::onRequestFinished() {
    if (status_ == Idle) {
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    if (reply_->error() == QNetworkReply::NoError) {
        QTimer::singleShot(10, this, SLOT(finish()));
    }
}

void Requestor::onRequestError(QNetworkReply::NetworkError error) {
    qWarning() << "O2Requestor::onRequestError: Error" << (int)error;
    if (status_ == Idle) {
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    qWarning() << "O2Requestor::onRequestError: Error string: " << reply_->errorString();
    int httpStatus = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qWarning() << "O2Requestor::onRequestError: HTTP status" << httpStatus << reply_->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if ((status_ == Requesting) && (httpStatus == 401)) {
        // Call OAuth2::refresh. Note the O2 instance might live in a different thread
        if (QMetaObject::invokeMethod(authenticator_, "refresh")) {
            return;
        }
        qCritical() << "O2Requestor::onRequestError: Invoking remote refresh failed";
    }
    error_ = error;
    QTimer::singleShot(10, this, SLOT(finish()));
}

void Requestor::onSslErrors(QList<QSslError> errors) {
    int i = 1;
    for (auto error : errors) {
        qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

void Requestor::onUploadProgress(qint64 uploaded, qint64 total) {
    if (status_ == Idle) {
        qWarning() << "O2Requestor::onUploadProgress: No pending request";
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    // Restart timeout because request in progress
    Reply *o2Reply = timedReplies_.find(reply_);
    if(o2Reply)
        o2Reply->start();
    emit uploadProgress(id_, uploaded, total);
}

int Requestor::setup(const QNetworkRequest &req, QNetworkAccessManager::Operation operation, const QByteArray &verb) {
    static int currentId;

    if (status_ != Idle) {
        qWarning() << "O2Requestor::setup: Another request pending";
        return -1;
    }

    request_ = req;
    operation_ = operation;
    id_ = currentId++;
    url_ = req.url();

    QUrl url = url_;
    if (addAccessTokenInQuery_) {
        QUrlQuery query(url);
        query.addQueryItem(OAUTH2_ACCESS_TOKEN, authenticator_->token());
        url.setQuery(query);
    }

    request_.setUrl(url);

    // If the service require the access token to be sent as a Authentication HTTP header, we add the access token.
    if (!accessTokenInAuthenticationHTTPHeaderFormat_.isEmpty()) {
        request_.setRawHeader(HTTP_AUTHORIZATION_HEADER, accessTokenInAuthenticationHTTPHeaderFormat_.arg(authenticator_->token()).toLatin1());
    }

    if (!verb.isEmpty()) {
        request_.setRawHeader(HTTP_HTTP_HEADER, verb);
    }

    status_ = Requesting;
    error_ = QNetworkReply::NoError;
    return id_;
}

void Requestor::finish() {
    QByteArray data;
    if (status_ == Idle) {
        qWarning() << "O2Requestor::finish: No pending request";
        return;
    }
    data = reply_->readAll();
    status_ = Idle;
    timedReplies_.remove(reply_);
    reply_->disconnect(this);
    reply_->deleteLater();
    QList<QNetworkReply::RawHeaderPair> headers = reply_->rawHeaderPairs();
    emit finished(id_, error_, data, headers);
}

void Requestor::retry() {
    if (status_ != Requesting) {
        qWarning() << "O2Requestor::retry: No pending request";
        return;
    }
    timedReplies_.remove(reply_);
    reply_->disconnect(this);
    reply_->deleteLater();
    QUrl url = url_;
    if (addAccessTokenInQuery_) {
        QUrlQuery query(url);
        query.addQueryItem(OAUTH2_ACCESS_TOKEN, authenticator_->token());
        url.setQuery(query);
    }
    request_.setUrl(url);

    // If the service require the access token to be sent as a Authentication HTTP header,
    // we update the access token when retrying.
    if(!accessTokenInAuthenticationHTTPHeaderFormat_.isEmpty()) {
        request_.setRawHeader(HTTP_AUTHORIZATION_HEADER, accessTokenInAuthenticationHTTPHeaderFormat_.arg(authenticator_->token()).toLatin1());
    }

    status_ = ReRequesting;
    switch (operation_) {
    case QNetworkAccessManager::GetOperation:
        reply_ = manager_->get(request_);
        break;
    case QNetworkAccessManager::PostOperation:
        reply_ = rawData_ ? manager_->post(request_, data_) : manager_->post(request_, multipartData_);
        break;
    case QNetworkAccessManager::CustomOperation:
    {
        QBuffer * buffer = new QBuffer;
        buffer->setData(data_);
        reply_ = manager_->sendCustomRequest(request_, request_.rawHeader(HTTP_HTTP_HEADER), buffer);
        buffer->setParent(reply_);
    }
        break;
    case QNetworkAccessManager::PutOperation:
        reply_ = rawData_ ? manager_->post(request_, data_) : manager_->put(request_, multipartData_);
        break;
    case QNetworkAccessManager::HeadOperation:
        reply_ = manager_->head(request_);
        break;
    default:
        assert(!"Unspecified operation for request");
        reply_ = manager_->get(request_);
        break;
    }
    timedReplies_.add(reply_);
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()), Qt::QueuedConnection);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
}

}
