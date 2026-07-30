// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vishrut Sachan <vishrutsachan2004@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <FileSystem.h>
#include <minecraft/mod/BisectController.h>
#include <minecraft/mod/ModFolderModel.h>

namespace {
void waitForUpdate(ModFolderModel& model, const std::function<void()>& trigger)
{
    QEventLoop loop;
    QObject::connect(&model, &ResourceFolderModel::updateFinished, &loop, &QEventLoop::quit);
    QTimer expireTimer;
    expireTimer.setSingleShot(true);
    QObject::connect(&expireTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    expireTimer.start(4000);
    trigger();
    loop.exec();
    QVERIFY2(expireTimer.isActive(), "Timer expired: update never finished.");
    expireTimer.stop();
}
}  // namespace

class BisectControllerTest : public QObject {
    Q_OBJECT

   private slots:
    void test_singleCulprit()
    {
        QTemporaryDir tmp;
        ModFolderModel model(tmp.path(), nullptr, false, true);

        for (const auto& name : { "alpha", "bravo", "charlie", "delta", "echo" }) {
            auto src = QFINDTESTDATA(QStringLiteral("testdata/Bisect/%1.jar").arg(name));
            waitForUpdate(model, [&] { model.installResource(src); });
        }
        QCOMPARE(model.size(), 5);

        QList<Mod*> allMods;
        for (int i = 0; i < model.size(); ++i)
            allMods << &model.at(i);

        // Planted culprit: "charlie". No locked mods for this run.
        auto* bisect = new BisectController(nullptr, &model, {}, allMods, this);
        QSignalSpy finishedSpy(bisect, &BisectController::finished);
        QSignalSpy readySpy(bisect, &BisectController::readyToLaunch);
        QSignalSpy promptSpy(bisect, &BisectController::promptUser);

        connect(bisect, &BisectController::readyToLaunch, bisect, [bisect] { bisect->onLaunchEnded(nullptr, true); });

        auto answerBasedOnCulprit = [&] {
            bool culpritEnabled = false;
            for (auto* mod : allMods) {
                if (mod->mod_id() == "charlie" && mod->enabled())
                    culpritEnabled = true;
            }
            bisect->onUserAnswered(culpritEnabled ? BisectController::Answer::Yes : BisectController::Answer::No);
        };

        bisect->start();
        for (int round = 0; round < 20 && finishedSpy.isEmpty(); ++round) {
            QVERIFY2(promptSpy.count() > round, "Bisect stopped prompting before converging.");
            answerBasedOnCulprit();
        }

        QCOMPARE(finishedSpy.count(), 1);
        auto culprits = finishedSpy.takeFirst().at(0).value<QList<Mod*>>();
        QCOMPARE(culprits.size(), 1);
        QCOMPARE(culprits.first()->mod_id(), QStringLiteral("charlie"));
    }
};

QTEST_GUILESS_MAIN(BisectControllerTest)

#include "BisectController_test.moc"
