#include "MojangRefresh.h"

MojangRefresh::MojangRefresh(AccountData* data, QObject* parent) : AuthContext(data, parent) {}

void MojangRefresh::executeTask() {
    m_requestsDone = 0;
    m_xboxProfileSucceeded = false;
    m_mcAuthSucceeded = false;

    initMojang();

    beginActivity(Katabasis::Activity::Refreshing);
    m_yggdrasil->refresh();
}
