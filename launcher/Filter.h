#pragma once

#include <QRegularExpression>
#include <QString>

class Filter {
   public:
    virtual ~Filter() = default;
    virtual bool accepts(const QString& value) = 0;
};

class ContainsFilter : public Filter {
   public:
    ContainsFilter(const QString& pattern);
    virtual ~ContainsFilter() = default;
    bool accepts(const QString& value) override;

   private:
    QString pattern;
};

class ExactFilter : public Filter {
   public:
    ExactFilter(const QString& pattern);
    virtual ~ExactFilter() = default;
    bool accepts(const QString& value) override;

   private:
    QString pattern;
};

class ExactIfPresentFilter : public Filter {
   public:
    ExactIfPresentFilter(const QString& pattern);
    virtual ~ExactIfPresentFilter() override = default;
    bool accepts(const QString& value) override;

   private:
    QString pattern;
};

class RegexpFilter : public Filter {
   public:
    RegexpFilter(const QString& regexp, bool invert);
    virtual ~RegexpFilter() = default;
    bool accepts(const QString& value) override;

   private:
    QRegularExpression pattern;
    bool invert = false;
};

class ExactListFilter : public Filter {
   public:
    ExactListFilter(const QStringList& pattern = {});
    virtual ~ExactListFilter() = default;
    bool accepts(const QString& value) override;

   private:
    QStringList m_pattern;
};
