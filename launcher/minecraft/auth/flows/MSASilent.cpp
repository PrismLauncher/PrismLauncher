#include "MSASilent.h"

MSASilent::MSASilent(AccountData* data, QObject* parent) : AuthContext(data, parent) {}

void MSASilent::executeTask() {
    m_requestsDone = 0;
    m_xboxProfileSucceeded = false;
    m_mcAuthSucceeded = false;

    initMSA();

    beginActivity(Katabasis::Activity::Refreshing);
    if(!m_oauth2->refresh()) {
        finishActivity();
    }
}
