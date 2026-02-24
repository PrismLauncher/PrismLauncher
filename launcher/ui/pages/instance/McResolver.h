#include <QtNetwork/qtcpsocket.h>
#include <QDnsLookup>
#include <QHostInfo>
#include <QObject>
#include <QString>

// resolve the IP and port of a Minecraft server
class McResolver : public QObject {
    Q_OBJECT

    QString m_constrDomain;
    int m_constrPort;

   public:
    explicit McResolver(QObject* parent, QString domain, int port);
    void ping();

   private:
    void pingWithDomainSRV(const QString& domain, int port);
    void pingWithDomainA(const QString& domain, int port);
    void emitFail(const QString& error);
    void emitSucceed(QString ip, int port);

   signals:
    void succeeded(const QString& ip, int port);
    void failed(const QString& error);
    void finished();
};
