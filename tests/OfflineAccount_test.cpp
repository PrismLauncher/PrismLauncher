// SPDX-License-Identifier: GPL-3.0-only
#include <QTest>

#include "LaunchController.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/AuthSession.h"

class OfflineAccountTest : public QObject {
    Q_OBJECT
   private slots:
    void emptyListIsNotValid()
    {
        AccountList accounts;
        QVERIFY(!accounts.anyAccountIsValid());
    }

    void microsoftAccountStillNeedsOwnership()
    {
        AccountList accounts;
        const auto account = MinecraftAccount::createBlankMSA();
        accounts.addAccount(account);
        QVERIFY(!accounts.anyAccountIsValid());

        LaunchController controller;
        controller.setAccountToUse(account);
        QCOMPARE(controller.decideLaunchMode(), LaunchDecision::Continue);
        QCOMPARE(controller.m_actualLaunchMode, LaunchMode::Demo);
    }

    void offlineAccountIsValidWithoutMicrosoft()
    {
        AccountList accounts;
        const auto account = MinecraftAccount::createOffline("LocalPlayer");
        account->login()->start();
        accounts.addAccount(account);
        accounts.setDefaultAccount(account);

        QCOMPARE(accounts.count(), 1);
        QVERIFY(accounts.anyAccountIsValid());
        QVERIFY(!account->ownsMinecraft());
        QCOMPARE(accounts.defaultAccount(), account);
    }

    void offlineLaunch_data()
    {
        QTest::addColumn<LaunchMode>("requested");
        QTest::newRow("normal-button") << LaunchMode::Normal;
        QTest::newRow("offline-button") << LaunchMode::Offline;
    }

    void offlineLaunch()
    {
        QFETCH(LaunchMode, requested);
        const auto account = MinecraftAccount::createOffline("LocalPlayer");
        LaunchController controller;
        controller.setAccountToUse(account);
        controller.setLaunchMode(requested);

        QCOMPARE(controller.decideLaunchMode(), LaunchDecision::Continue);
        QCOMPARE(controller.m_actualLaunchMode, requested);
        QVERIFY(!account->isActive());
    }

    void explicitDemoIsPreserved()
    {
        LaunchController controller;
        controller.setAccountToUse(MinecraftAccount::createOffline("LocalPlayer"));
        controller.setLaunchMode(LaunchMode::Demo);
        QCOMPARE(controller.decideLaunchMode(), LaunchDecision::Continue);
        QCOMPARE(controller.m_actualLaunchMode, LaunchMode::Demo);
    }

    void missingAccountUsesDemo()
    {
        LaunchController controller;
        QCOMPARE(controller.decideLaunchMode(), LaunchDecision::Continue);
        QCOMPARE(controller.m_actualLaunchMode, LaunchMode::Demo);
    }

    void offlineProfileSurvivesReload()
    {
        const auto account = MinecraftAccount::createOffline("LocalPlayer");
        const auto restored = MinecraftAccount::loadFromJsonV3(account->saveToJson());
        QVERIFY(restored);
        QCOMPARE(restored->accountType(), AccountType::Offline);
        QCOMPARE(restored->profileId(), account->profileId());
        QCOMPARE(restored->profileName(), QString("LocalPlayer"));

        AccountList accounts;
        accounts.addAccount(restored);
        QVERIFY(accounts.anyAccountIsValid());

        LaunchController controller;
        controller.setAccountToUse(restored);
        QCOMPARE(controller.decideLaunchMode(), LaunchDecision::Continue);
        QCOMPARE(controller.m_actualLaunchMode, LaunchMode::Normal);

        const auto session = std::make_shared<AuthSession>();
        session->launchMode = controller.m_actualLaunchMode;
        restored->fillSession(session);
        QCOMPARE(session->launchMode, LaunchMode::Normal);
        QCOMPARE(session->player_name, QString("LocalPlayer"));
        QCOMPARE(session->uuid, account->profileId());
        QCOMPARE(session->user_type, QString("offline"));
        QCOMPARE(session->access_token, QString("0"));
    }
};

QTEST_MAIN(OfflineAccountTest)
#include "OfflineAccount_test.moc"
