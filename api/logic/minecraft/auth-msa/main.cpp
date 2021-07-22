#include <QApplication>
#include <QStringList>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QSaveFile>

#include "context.h"
#include "mainwindow.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

class Helper : public QObject {
    Q_OBJECT

public:
    Helper(Context * context) : QObject(), context_(context), msg_(QString()) {
        QFile tokenCache("usercache.dat");
        if(tokenCache.open(QIODevice::ReadOnly)) {
            context_->resumeFromState(tokenCache.readAll());
        }
    }

public slots:
    void run() {
        connect(context_, &Context::activityChanged, this, &Helper::onActivityChanged);
        context_->silentSignIn();
    }

    void onFailed() {
        qDebug() << "Login failed";
    }

    void onActivityChanged(Katabasis::Activity activity) {
        if(activity == Katabasis::Activity::Idle) {
            switch(context_->validity()) {
                case Katabasis::Validity::None: {
                    // account is gone, remove it.
                    QFile::remove("usercache.dat");
                }
                break;
                case Katabasis::Validity::Assumed: {
                    // this is basically a soft-failed refresh. do nothing.
                }
                break;
                case Katabasis::Validity::Certain: {
                    // stuff got refreshed / signed in. Save.
                    auto data = context_->saveState();
                    QSaveFile tokenCache("usercache.dat");
                    if(tokenCache.open(QIODevice::WriteOnly)) {
                        tokenCache.write(context_->saveState());
                        tokenCache.commit();
                    }
                }
                break;
            }
        }
    }

private:
    Context *context_;
    QString msg_;
};

int main(int argc, char *argv[]) {
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("MultiMC");
    QCoreApplication::setApplicationName("MultiMC");
    Context c;
    Helper helper(&c);
    MainWindow window(&c);
    window.show();
    QTimer::singleShot(0, &helper, &Helper::run);
    return a.exec();
}

#include "main.moc"
