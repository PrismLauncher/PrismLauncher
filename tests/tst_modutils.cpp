/* Copyright 2013-2015 MultiMC Contributors
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

#include "modutils.h"
#include "TestUtil.h"

class ModUtilsTest : public QObject
{
	Q_OBJECT
	void setupVersions()
	{
		QTest::addColumn<QString>("first");
		QTest::addColumn<QString>("second");
		QTest::addColumn<bool>("lessThan");
		QTest::addColumn<bool>("equal");

		QTest::newRow("equal, explicit") << "1.2.0" << "1.2.0" << false << true;
		QTest::newRow("equal, implicit 1") << "1.2" << "1.2.0" << false << true;
		QTest::newRow("equal, implicit 2") << "1.2.0" << "1.2" << false << true;
		QTest::newRow("equal, two-digit") << "1.42" << "1.42" << false << true;

		QTest::newRow("lessThan, explicit 1") << "1.2.0" << "1.2.1" << true << false;
		QTest::newRow("lessThan, explicit 2") << "1.2.0" << "1.3.0" << true << false;
		QTest::newRow("lessThan, explicit 3") << "1.2.0" << "2.2.0" << true << false;
		QTest::newRow("lessThan, implicit 1") << "1.2" << "1.2.1" << true << false;
		QTest::newRow("lessThan, implicit 2") << "1.2" << "1.3.0" << true << false;
		QTest::newRow("lessThan, implicit 3") << "1.2" << "2.2.0" << true << false;
		QTest::newRow("lessThan, two-digit") << "1.41" << "1.42" << true << false;

		QTest::newRow("greaterThan, explicit 1") << "1.2.1" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, explicit 2") << "1.3.0" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, explicit 3") << "2.2.0" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, implicit 1") << "1.2.1" << "1.2" << false << false;
		QTest::newRow("greaterThan, implicit 2") << "1.3.0" << "1.2" << false << false;
		QTest::newRow("greaterThan, implicit 3") << "2.2.0" << "1.2" << false << false;
		QTest::newRow("greaterThan, two-digit") << "1.42" << "1.41" << false << false;
	}

private slots:
	void initTestCase()
	{

	}
	void cleanupTestCase()
	{

	}

	void test_versionIsInInterval_data()
	{
		QTest::addColumn<QString>("version");
		QTest::addColumn<QString>("interval");
		QTest::addColumn<bool>("result");

		QTest::newRow("empty, true") << "1.2.3" << "" << true;
		QTest::newRow("one version, true") << "1.2.3" << "1.2.3" << true;
		QTest::newRow("one version, false") << "1.2.3" << "1.2.2" << false;

		QTest::newRow("one version inclusive <-> infinity, true") << "1.2.3" << "[1.2.3,)" << true;
		QTest::newRow("one version exclusive <-> infinity, true") << "1.2.3" << "(1.2.2,)" << true;
		QTest::newRow("one version inclusive <-> infitity, false") << "1.2.3" << "[1.2.4,)" << false;
		QTest::newRow("one version exclusive <-> infinity, false") << "1.2.3" << "(1.2.3,)" << false;

		QTest::newRow("infinity <-> one version inclusive, true") << "1.2.3" << "(,1.2.3]" << true;
		QTest::newRow("infinity <-> one version exclusive, true") << "1.2.3" << "(,1.2.4)" << true;
		QTest::newRow("infinity <-> one version inclusive, false") << "1.2.3" << "(,1.2.2]" << false;
		QTest::newRow("infinity <-> one version exclusive, false") << "1.2.3" << "(,1.2.3)" << false;

		QTest::newRow("inclusive <-> inclusive, true") << "1.2.3" << "[1.2.2,1.2.3]" << true;
		QTest::newRow("inclusive <-> exclusive, true") << "1.2.3" << "[1.2.3,1.2.4)" << true;
		QTest::newRow("exclusive <-> inclusive, true") << "1.2.3" << "(1.2.2,1.2.3]" << true;
		QTest::newRow("exclusive <-> exclusive, true") << "1.2.3" << "(1.2.2,1.2.4)" << true;
		QTest::newRow("inclusive <-> inclusive, false") << "1.2.3" << "[1.0.0,1.2.2]" << false;
		QTest::newRow("inclusive <-> exclusive, false") << "1.2.3" << "[1.0.0,1.2.3)" << false;
		QTest::newRow("exclusive <-> inclusive, false") << "1.2.3" << "(1.2.3,2.0.0]" << false;
		QTest::newRow("exclusive <-> exclusive, false") << "1.2.3" << "(1.0.0,1.2.3)" << false;
	}
	void test_versionIsInInterval()
	{
		QFETCH(QString, version);
		QFETCH(QString, interval);
		QFETCH(bool, result);

		QCOMPARE(Util::versionIsInInterval(version, interval), result);
	}

	void test_versionCompare_data()
	{
		setupVersions();
	}
	void test_versionCompare()
	{
		QFETCH(QString, first);
		QFETCH(QString, second);
		QFETCH(bool, lessThan);
		QFETCH(bool, equal);

		const auto v1 = Util::Version(first);
		const auto v2 = Util::Version(second);

		QCOMPARE(v1 < v2, lessThan);
		QCOMPARE(v1 > v2, !lessThan && !equal);
		QCOMPARE(v1 == v2, equal);
	}
};

QTEST_GUILESS_MAIN(ModUtilsTest)

#include "tst_modutils.moc"
