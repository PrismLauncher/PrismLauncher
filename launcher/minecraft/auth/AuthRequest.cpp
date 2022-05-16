#include <cassert>

#include <QDebug>
#include <QTimer>
#include <QBuffer>
#include <QUrlQuery>

#include "Application.h"
#include "AuthRequest.h"
#include "katabasis/Globals.h"

AuthRequest::AuthRequest(QObject *parent): QObject(parent) {
}

AuthRequest::~AuthRequest() {
}

void AuthRequest::get(const QNetworkRequest &req, int timeout/* = 60*1000*/) {
    setup(req, QNetworkAccessManager::GetOperation);
    reply_ = APPLICATION->network()->get(request_);
    status_ = Requesting;
    timedReplies_.add(new Katabasis::Reply(reply_, timeout));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(reply_, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
#else
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
#endif
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()));
    connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
}

void AuthRequest::post(const QNetworkRequest &req, const QByteArray &data, int timeout/* = 60*1000*/) {
    setup(req, QNetworkAccessManager::PostOperation);
    data_ = data;
    status_ = Requesting;
    reply_ = APPLICATION->network()->post(request_, data_);
    timedReplies_.add(new Katabasis::Reply(reply_, timeout));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(reply_, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
#else
    connect(reply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRequestError(QNetworkReply::NetworkError)));
#endif
    connect(reply_, SIGNAL(finished()), this, SLOT(onRequestFinished()));
    connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
    connect(reply_, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
}

void AuthRequest::onRequestFinished() {
    if (status_ == Idle) {
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    httpStatus_ = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    finish();
}

void AuthRequest::onRequestError(QNetworkReply::NetworkError error) {
    qWarning() << "AuthRequest::onRequestError: Error" << (int)error;
    if (status_ == Idle) {
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    errorString_ = reply_->errorString();
    httpStatus_ = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    error_ = error;
    qWarning() << "AuthRequest::onRequestError: Error string: " << errorString_;
    qWarning() << "AuthRequest::onRequestError: HTTP status" << httpStatus_ << reply_->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    // QTimer::singleShot(10, this, SLOT(finish()));
}

void AuthRequest::onSslErrors(QList<QSslError> errors) {
    int i = 1;
    for (auto error : errors) {
        qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

void AuthRequest::onUploadProgress(qint64 uploaded, qint64 total) {
    if (status_ == Idle) {
        qWarning() << "AuthRequest::onUploadProgress: No pending request";
        return;
    }
    if (reply_ != qobject_cast<QNetworkReply *>(sender())) {
        return;
    }
    // Restart timeout because request in progress
    Katabasis::Reply *o2Reply = timedReplies_.find(reply_);
    if(o2Reply) {
        o2Reply->start();
    }
    emit uploadProgress(uploaded, total);
}

void AuthRequest::setup(const QNetworkRequest &req, QNetworkAccessManager::Operation operation, const QByteArray &verb) {
    request_ = req;
    operation_ = operation;
    url_ = req.url();

    QUrl url = url_;
    request_.setUrl(url);

    if (!verb.isEmpty()) {
        request_.setRawHeader(Katabasis::HTTP_HTTP_HEADER, verb);
    }

    status_ = Requesting;
    error_ = QNetworkReply::NoError;
    errorString_.clear();
    httpStatus_ = 0;
}

void AuthRequest::finish() {
    QByteArray data;
    if (status_ == Idle) {
        qWarning() << "AuthRequest::finish: No pending request";
        return;
    }
    data = reply_->readAll();
    status_ = Idle;
    timedReplies_.remove(reply_);
    reply_->disconnect(this);
    reply_->deleteLater();
    QList<QNetworkReply::RawHeaderPair> headers = reply_->rawHeaderPairs();
    emit finished(error_, data, headers);
}
