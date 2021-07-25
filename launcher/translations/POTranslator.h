#pragma once

#include <QTranslator>

struct POTranslatorPrivate;

class POTranslator : public QTranslator
{
    Q_OBJECT
public:
    explicit POTranslator(const QString& filename, QObject * parent = nullptr);
    QString translate(const char * context, const char * sourceText, const char * disambiguation, int n) const override;
    bool isEmpty() const override;
private:
    POTranslatorPrivate * d;
};
