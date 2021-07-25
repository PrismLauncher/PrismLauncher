#pragma once

#include <QString>
#include <QRegularExpression>

class Filter
{
public:
    virtual ~Filter();
    virtual bool accepts(const QString & value) = 0;
};

class ContainsFilter: public Filter
{
public:
    ContainsFilter(const QString &pattern);
    virtual ~ContainsFilter();
    bool accepts(const QString & value) override;
private:
    QString pattern;
};

class ExactFilter: public Filter
{
public:
    ExactFilter(const QString &pattern);
    virtual ~ExactFilter();
    bool accepts(const QString & value) override;
private:
    QString pattern;
};

class RegexpFilter: public Filter
{
public:
    RegexpFilter(const QString &regexp, bool invert);
    virtual ~RegexpFilter();
    bool accepts(const QString & value) override;
private:
    QRegularExpression pattern;
    bool invert = false;
};
