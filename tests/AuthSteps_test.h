#pragma once

#include <QObject>

class AuthStepsTest : public QObject {
    Q_OBJECT

   private slots:
    void test_msaClientIdFromInjectedSettings();
    void test_offlineFlowSucceedsWithoutServices();
    void test_refreshWithChangedClientIdIsDisabled();
    void test_xboxUserStepUsesInjectedNetwork();
    void test_deviceCodeStepSendsInjectedClientId();
};
