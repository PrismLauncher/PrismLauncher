#include "MojangLogin.h"

MojangLogin::MojangLogin(
    AccountData *data,
    QString password,
    QObject *parent
): AuthContext(data, parent), m_password(password) {}

void MojangLogin::executeTask() {
    m_requestsDone = 0;
    m_xboxProfileSucceeded = false;
    m_mcAuthSucceeded = false;

    initMojang();

    beginActivity(Katabasis::Activity::LoggingIn);
    m_yggdrasil->login(m_password);
}
