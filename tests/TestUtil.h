#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>

#include "MultiMC.h"

struct TestsInternal
{
        static QByteArray readFile(const QString &fileName)
        {
                QFile f(fileName);
                f.open(QFile::ReadOnly);
                return f.readAll();
        }
		static QString readFileUtf8(const QString &fileName)
		{
			return QString::fromUtf8(readFile(fileName));
		}
};

#define MULTIMC_GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA( file ))
#define MULTIMC_GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA( file ))

#define QTEST_GUILESS_MAIN_MULTIMC(TestObject) \
int main(int argc, char *argv[]) \
{ \
	char *argv_[] = { argv[0] }; \
	int argc_ = 1; \
	MultiMC app(argc_, argv_, QDir::temp().absoluteFilePath("MultiMC_Test")); \
	app.setAttribute(Qt::AA_Use96Dpi, true); \
	TestObject tc; \
	return QTest::qExec(&tc, argc, argv); \
}
