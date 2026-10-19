// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>

#include "plugins/lua/PrismApi.h"

class QNetworkAccessManager;

// Sends the plugins' requests with the launcher's network access manager, so they use its proxy settings
class NetworkHttpClient : public PrismApi::HttpClient {
   public:
    NetworkHttpClient(QNetworkAccessManager* network, const QByteArray& userAgent);

    void send(const PrismApi::HttpRequest& request, std::function<void(const PrismApi::HttpResponse&)> done) override;

   private:
    QPointer<QNetworkAccessManager> m_network;
    QByteArray m_userAgent;
};
