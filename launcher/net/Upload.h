#pragma once

#include "NetAction.h"
#include "Sink.h"

namespace Net {

    class Upload : public NetAction {
        Q_OBJECT

    public:
        static Upload::Ptr makeByteArray(QUrl url, QByteArray *output, QByteArray m_post_data);

    protected slots:
        void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
        void downloadError(QNetworkReply::NetworkError error) override;
        void sslErrors(const QList<QSslError> & errors);
        void downloadFinished() override;
        void downloadReadyRead() override;

    public slots:
        void executeTask() override;
    private:
        std::unique_ptr<Sink> m_sink;
        QByteArray m_post_data;

        bool handleRedirect();
    };

} // Net

