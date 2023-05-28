// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // QNetworkReply::errorOccurred added in 5.15
    connect(reply_, &QNetworkReply::errorOccurred, this, &AuthRequest::onRequestError);
#else // &QNetworkReply::error SIGNAL depricated
    connect(reply_, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &AuthRequest::onRequestError);
#endif
    connect(reply_, &QNetworkReply::finished, this, &AuthRequest::onRequestFinished);
    connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
}

void AuthRequest::post(const QNetworkRequest &req, const QByteArray &data, int timeout/* = 60*1000*/) {
    setup(req, QNetworkAccessManager::PostOperation);
    data_ = data;
    status_ = Requesting;
    reply_ = APPLICATION->network()->post(request_, data_);
    timedReplies_.add(new Katabasis::Reply(reply_, timeout));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // QNetworkReply::errorOccurred added in 5.15
    connect(reply_, &QNetworkReply::errorOccurred, this, &AuthRequest::onRequestError);
#else // &QNetworkReply::error SIGNAL depricated
    connect(reply_, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &AuthRequest::onRequestError);
#endif
    connect(reply_, &QNetworkReply::finished, this, &AuthRequest::onRequestFinished);
    connect(reply_, &QNetworkReply::sslErrors, this, &AuthRequest::onSslErrors);
    connect(reply_, &QNetworkReply::uploadProgress, this, &AuthRequest::onUploadProgress);
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
