#pragma once

#include <QFile>
#include <QCoreApplication>
#include <QTest>
#include <QDir>

#define expandstr(s) expandstr2(s)
#define expandstr2(s) #s

class TestsInternal
{
public:
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

#define GET_TEST_FILE(file) TestsInternal::readFile(QFINDTESTDATA(file))
#define GET_TEST_FILE_UTF8(file) TestsInternal::readFileUtf8(QFINDTESTDATA(file))

