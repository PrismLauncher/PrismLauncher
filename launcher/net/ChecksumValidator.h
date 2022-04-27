#pragma once

#include "Validator.h"

#include <QCryptographicHash>
#include <QFile>

namespace Net {
class ChecksumValidator : public Validator {
   public:
    ChecksumValidator(QCryptographicHash::Algorithm algorithm, QByteArray expected = QByteArray())
        : m_checksum(algorithm), m_expected(expected){};
    virtual ~ChecksumValidator() = default;

   public:
    auto init(QNetworkRequest&) -> bool override
    {
        m_checksum.reset();
        return true;
    }

    auto write(QByteArray& data) -> bool override
    {
        m_checksum.addData(data);
        return true;
    }

    auto abort() -> bool override { return true; }

    auto validate(QNetworkReply&) -> bool override
    {
        if (m_expected.size() && m_expected != hash()) {
            qWarning() << "Checksum mismatch, download is bad.";
            return false;
        }
        return true;
    }

    auto hash() -> QByteArray { return m_checksum.result(); }

    void setExpected(QByteArray expected) { m_expected = expected; }

   private:
    QCryptographicHash m_checksum;
    QByteArray m_expected;
};
}  // namespace Net
