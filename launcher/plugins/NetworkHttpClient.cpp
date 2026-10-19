// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "NetworkHttpClient.h"

#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

NetworkHttpClient::NetworkHttpClient(QNetworkAccessManager* network, const QByteArray& userAgent)
    : m_network(network), m_userAgent(userAgent)
{}

void NetworkHttpClient::send(const PrismApi::HttpRequest& request, std::function<void(const PrismApi::HttpResponse&)> done)
{
    if (!m_network) {
        QTimer::singleShot(0, [done] { done({ 0, {}, {}, "the network is not available" }); });
        return;
    }

    QNetworkRequest networkRequest(request.url);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    for (const auto& [name, value] : request.headers) {
        networkRequest.setRawHeader(name, value);
    }
    networkRequest.setTransferTimeout(request.timeoutMs);
    // Never follow a redirect from https to http
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_network->sendCustomRequest(networkRequest, request.method, request.body);
    auto tooLarge = std::make_shared<bool>(false);
    QObject::connect(reply, &QNetworkReply::downloadProgress, reply, [reply, tooLarge](qint64 received, qint64) {
        if (received > PrismApi::s_maxHttpResponseSize && !*tooLarge) {
            *tooLarge = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, tooLarge, done] {
        PrismApi::HttpResponse response;
        response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (*tooLarge) {
            response.error = QString("the response is larger than %1 bytes").arg(PrismApi::s_maxHttpResponseSize);
        } else if (response.status == 0) {
            response.error = reply->errorString();
        } else {
            response.body = reply->readAll();
            for (const auto& [name, value] : reply->rawHeaderPairs()) {
                response.headers.append({ name.toLower(), value });
            }
        }
        reply->deleteLater();
        done(response);
    });
}
