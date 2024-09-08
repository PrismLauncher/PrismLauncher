/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QTest>

#include <Version.h>

class VersionTest : public QObject {
    Q_OBJECT

    QStringList m_flex_test_names = {};

    void addDataColumns()
    {
        QTest::addColumn<QString>("first");
        QTest::addColumn<QString>("second");
        QTest::addColumn<bool>("lessThan");
        QTest::addColumn<bool>("equal");
    }

    void setupVersions()
    {
        addDataColumns();

        QTest::newRow("equal, explicit") << "1.2.0"
                                         << "1.2.0" << false << true;
        QTest::newRow("equal, two-digit") << "1.42"
                                          << "1.42" << false << true;

        QTest::newRow("lessThan, explicit 1") << "1.2.0"
                                              << "1.2.1" << true << false;
        QTest::newRow("lessThan, explicit 2") << "1.2.0"
                                              << "1.3.0" << true << false;
        QTest::newRow("lessThan, explicit 3") << "1.2.0"
                                              << "2.2.0" << true << false;
        QTest::newRow("lessThan, implicit 1") << "1.2"
                                              << "1.2.0" << true << false;
        QTest::newRow("lessThan, implicit 2") << "1.2"
                                              << "1.2.1" << true << false;
        QTest::newRow("lessThan, implicit 3") << "1.2"
                                              << "1.3.0" << true << false;
        QTest::newRow("lessThan, implicit 4") << "1.2"
                                              << "2.2.0" << true << false;
        QTest::newRow("lessThan, two-digit") << "1.41"
                                             << "1.42" << true << false;
        QTest::newRow("lessThan, snapshot") << "1.20.0-rc2"
                                            << "1.20.1" << true << false;

        QTest::newRow("greaterThan, explicit 1") << "1.2.1"
                                                 << "1.2.0" << false << false;
        QTest::newRow("greaterThan, explicit 2") << "1.3.0"
                                                 << "1.2.0" << false << false;
        QTest::newRow("greaterThan, explicit 3") << "2.2.0"
                                                 << "1.2.0" << false << false;
        QTest::newRow("greaterThan, implicit 1") << "1.2.0"
                                                 << "1.2" << false << false;
        QTest::newRow("greaterThan, implicit 2") << "1.2.1"
                                                 << "1.2" << false << false;
        QTest::newRow("greaterThan, implicit 3") << "1.3.0"
                                                 << "1.2" << false << false;
        QTest::newRow("greaterThan, implicit 4") << "2.2.0"
                                                 << "1.2" << false << false;
        QTest::newRow("greaterThan, two-digit") << "1.42"
                                                << "1.41" << false << false;
        QTest::newRow("greaterThan, snapshot") << "1.20.2-rc2"
                                               << "1.20.1" << false << false;
    }

   private slots:
    void test_versionCompare_data() { setupVersions(); }

    void test_versionCompare()
    {
        QFETCH(QString, first);
        QFETCH(QString, second);
        QFETCH(bool, lessThan);
        QFETCH(bool, equal);

        const auto v1 = Version(first);
        const auto v2 = Version(second);

        qDebug() << v1 << "vs" << v2;

        QCOMPARE(v1 < v2, lessThan);
        QCOMPARE(v1 > v2, !lessThan && !equal);
        QCOMPARE(v1 == v2, equal);
    }

    void test_flexVerTestVector_data()
    {
        addDataColumns();

        QDir test_vector_dir(QFINDTESTDATA("testdata/Version"));

        QFile vector_file{ test_vector_dir.absoluteFilePath("test_vectors.txt") };

        vector_file.open(QFile::OpenModeFlag::ReadOnly);

        int test_number = 0;
        const QString test_name_template{ "FlexVer test #%1 (%2)" };
        for (auto line = vector_file.readLine(); !vector_file.atEnd(); line = vector_file.readLine()) {
            line = line.simplified();
            if (line.startsWith('#') || line.isEmpty())
                continue;

            test_number += 1;

            auto split_line = line.split('<');
            if (split_line.size() == 2) {
                QString first{ split_line.first().simplified() };
                QString second{ split_line.last().simplified() };

                auto new_test_name = test_name_template.arg(QString::number(test_number), "lessThan");
                m_flex_test_names.append(new_test_name);
                QTest::newRow(m_flex_test_names.last().toLatin1().data()) << first << second << true << false;

                continue;
            }

            split_line = line.split('=');
            if (split_line.size() == 2) {
                QString first{ split_line.first().simplified() };
                QString second{ split_line.last().simplified() };

                auto new_test_name = test_name_template.arg(QString::number(test_number), "equals");
                m_flex_test_names.append(new_test_name);
                QTest::newRow(m_flex_test_names.last().toLatin1().data()) << first << second << false << true;

                continue;
            }

            split_line = line.split('>');
            if (split_line.size() == 2) {
                QString first{ split_line.first().simplified() };
                QString second{ split_line.last().simplified() };

                auto new_test_name = test_name_template.arg(QString::number(test_number), "greaterThan");
                m_flex_test_names.append(new_test_name);
                QTest::newRow(m_flex_test_names.last().toLatin1().data()) << first << second << false << false;

                continue;
            }

            qCritical() << "Unexpected separator in the test vector: ";
            qCritical() << line;

            QVERIFY(0 != 0);
        }

        vector_file.close();
    }

    void test_flexVerTestVector()
    {
        QFETCH(QString, first);
        QFETCH(QString, second);
        QFETCH(bool, lessThan);
        QFETCH(bool, equal);

        const auto v1 = Version(first);
        const auto v2 = Version(second);

        qDebug() << v1 << "vs" << v2;

        QCOMPARE(v1 < v2, lessThan);
        QCOMPARE(v1 > v2, !lessThan && !equal);
        QCOMPARE(v1 == v2, equal);
    }
};

QTEST_GUILESS_MAIN(VersionTest)

#include "Version_test.moc"
