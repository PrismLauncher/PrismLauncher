#include <cassert>

#include <QDebug>
#include <QTimer>
#include <QBuffer>
#include <QUrlQuery>

#include "katabasis/Requestor.h"
#include "katabasis/OAuth2.h"
#include "katabasis/Globals.h"

namespace Katabasis {

Requestor::Requestor(QNetworkAccessManager *manager, OAuth2 *authenticator, QObject *parent): QObject(parent), reply_(NULL), status_(Idle) {
    manager_ = manager;
    authenticator_ = authenticator;
    if (authenticator) {
        timedReplies_.setIgnoreSslErrors(authenticator->ignoreSslErrors());
    }
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
    connect(authenticator, &OAuth2::refreshFinished, this, &Requestor::onRefreshFinished);
}

Requestor::~Requestor() {
}

int Requestor::get(const QNetworkRequest &req, int timeout/* = 60*1000*/) {
    if (-1 == setup(req, QNetworkAccessManager::GetOperation)) {
        return -1;
    }
    reply_ = manager_->get(request_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()));
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    return id_;
}

int Requestor::post(const QNetworkRequest &req, const QByteArray &data, int timeout/* = 60*1000*/) {
    if (-1 == setup(req, QNetworkAccessManager::PostOperation)) {
        return -1;
    }
    data_ = data;
    reply_ = manager_->post(request_, data_);
    timedReplies_.add(new Reply(reply_, timeout));
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()));
    connect(reply_, &QNetworkReply::sslErrors, this, &Requestor::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
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
    request_.setUrl(url);

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
    request_.setUrl(url);

    status_ = ReRequesting;
    switch (operation_) {
    case QNetworkAccessManager::GetOperation:
        reply_ = manager_->get(request_);
        break;
    case QNetworkAccessManager::PostOperation:
        reply_ = manager_->post(request_, data_);
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
