#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QMap>
#include <QPair>
#include <QTimer>
#include <QStringList>
#include <QUrl>
#include <QDebug>
#include <QUrlQuery>

#include "katabasis/Globals.h"
#include "katabasis/ReplyServer.h"

namespace Katabasis {

ReplyServer::ReplyServer(QObject *parent): QTcpServer(parent),
  timeout_(15), maxtries_(3), tries_(0) {
    qDebug() << "O2ReplyServer: Starting";
    connect(this, SIGNAL(newConnection()), this, SLOT(onIncomingConnection()));
    replyContent_ = "<HTML></HTML>";
}

void ReplyServer::onIncomingConnection() {
    qDebug() << "O2ReplyServer::onIncomingConnection: Receiving...";
    QTcpSocket *socket = nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(onBytesReady()), Qt::UniqueConnection);
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    // Wait for a bit *after* first response, then close server if no useable data has arrived
    // Helps with implicit flow, where a URL fragment may need processed by local user-agent and
    // sent as secondary query string callback, or additional requests make it through first,
    // like for favicons, etc., before such secondary callbacks are fired
    QTimer *timer = new QTimer(socket);
    timer->setObjectName("timeoutTimer");
    connect(timer, SIGNAL(timeout()), this, SLOT(closeServer()));
    timer->setSingleShot(true);
    timer->setInterval(timeout() * 1000);
    connect(socket, SIGNAL(readyRead()), timer, SLOT(start()));
}

void ReplyServer::onBytesReady() {
    if (!isListening()) {
        // server has been closed, stop processing queued connections
        return;
    }
    qDebug() << "O2ReplyServer::onBytesReady: Processing request";
    // NOTE: on first call, the timeout timer is started
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        qWarning() << "O2ReplyServer::onBytesReady: No socket available";
        return;
    }
    QByteArray reply;
    reply.append("HTTP/1.0 200 OK \r\n");
    reply.append("Content-Type: text/html; charset=\"utf-8\"\r\n");
    reply.append(QString("Content-Length: %1\r\n\r\n").arg(replyContent_.size()).toLatin1());
    reply.append(replyContent_);
    socket->write(reply);
    qDebug() << "O2ReplyServer::onBytesReady: Sent reply";

    QByteArray data = socket->readAll();
    QMap<QString, QString> queryParams = parseQueryParams(&data);
    if (queryParams.isEmpty()) {
        if (tries_ < maxtries_ ) {
            qDebug() << "O2ReplyServer::onBytesReady: No query params found, waiting for more callbacks";
            ++tries_;
            return;
        } else {
            tries_ = 0;
            qWarning() << "O2ReplyServer::onBytesReady: No query params found, maximum callbacks received";
            closeServer(socket, false);
            return;
        }
    }
    if (!uniqueState_.isEmpty() && !queryParams.contains(QString(OAUTH2_STATE))) {
        qDebug() << "O2ReplyServer::onBytesReady: Malicious or service request";
        closeServer(socket, true);
        return; // Malicious or service (e.g. favicon.ico) request
    }
    qDebug() << "O2ReplyServer::onBytesReady: Query params found, closing server";
    closeServer(socket, true);
    emit verificationReceived(queryParams);
}

QMap<QString, QString> ReplyServer::parseQueryParams(QByteArray *data) {
    qDebug() << "O2ReplyServer::parseQueryParams";

    //qDebug() << QString("O2ReplyServer::parseQueryParams data:\n%1").arg(QString(*data));

    QString splitGetLine = QString(*data).split("\r\n").first();
    splitGetLine.remove("GET ");
    splitGetLine.remove("HTTP/1.1");
    splitGetLine.remove("\r\n");
    splitGetLine.prepend("http://localhost");
    QUrl getTokenUrl(splitGetLine);

    QList< QPair<QString, QString> > tokens;
    QUrlQuery query(getTokenUrl);
    tokens = query.queryItems();
    QMap<QString, QString> queryParams;
    QPair<QString, QString> tokenPair;
    foreach (tokenPair, tokens) {
        // FIXME: We are decoding key and value again. This helps with Google OAuth, but is it mandated by the standard?
        QString key = QUrl::fromPercentEncoding(QByteArray().append(tokenPair.first.trimmed().toLatin1()));
        QString value = QUrl::fromPercentEncoding(QByteArray().append(tokenPair.second.trimmed().toLatin1()));
        queryParams.insert(key, value);
    }
    return queryParams;
}

void ReplyServer::closeServer(QTcpSocket *socket, bool hasparameters)
{
  if (!isListening()) {
      return;
  }

  qDebug() << "O2ReplyServer::closeServer: Initiating";
  int port = serverPort();

  if (!socket && sender()) {
      QTimer *timer = qobject_cast<QTimer*>(sender());
      if (timer) {
          qWarning() << "O2ReplyServer::closeServer: Closing due to timeout";
          timer->stop();
          socket = qobject_cast<QTcpSocket *>(timer->parent());
          timer->deleteLater();
      }
  }
  if (socket) {
      QTimer *timer = socket->findChild<QTimer*>("timeoutTimer");
      if (timer) {
          qDebug() << "O2ReplyServer::closeServer: Stopping socket's timeout timer";
          timer->stop();
      }
      socket->disconnectFromHost();
  }
  close();
  qDebug() << "O2ReplyServer::closeServer: Closed, no longer listening on port" << port;
  emit serverClosed(hasparameters);
}

QByteArray ReplyServer::replyContent() {
    return replyContent_;
}

void ReplyServer::setReplyContent(const QByteArray &value) {
  replyContent_ = value;
}

int ReplyServer::timeout()
{
  return timeout_;
}

void ReplyServer::setTimeout(int timeout)
{
  timeout_ = timeout;
}

int ReplyServer::callbackTries()
{
  return maxtries_;
}

void ReplyServer::setCallbackTries(int maxtries)
{
  maxtries_ = maxtries;
}

QString ReplyServer::uniqueState()
{
    return uniqueState_;
}

void ReplyServer::setUniqueState(const QString &state)
{
    uniqueState_ = state;
}

}
