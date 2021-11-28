#include "MSAInteractive.h"

MSAInteractive::MSAInteractive(
    AccountData* data,
    QObject* parent
) : AuthContext(data, parent) {}

void MSAInteractive::executeTask() {
    m_requestsDone = 0;
    m_xboxProfileSucceeded = false;
    m_mcAuthSucceeded = false;

    initMSA();

    QVariantMap extraOpts;
    extraOpts["prompt"] = "select_account";
    m_oauth2->setExtraRequestParams(extraOpts);

    beginActivity(Katabasis::Activity::LoggingIn);
    *m_data = AccountData();
    m_oauth2->login();
}
