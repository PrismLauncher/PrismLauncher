#pragma once

#include "Validator.h"
#include <QCryptographicHash>
#include <memory>
#include <QFile>

namespace Net {
class ChecksumValidator: public Validator
{
public: /* con/des */
	ChecksumValidator(QCryptographicHash::Algorithm algorithm, QByteArray expected = QByteArray())
		:m_checksum(algorithm), m_expected(expected)
	{
	};
	virtual ~ChecksumValidator() {};

public: /* methods */
	bool init(QNetworkRequest &) override
	{
		m_checksum.reset();
		return true;
	}
	bool write(QByteArray & data) override
	{
		m_checksum.addData(data);
		this->data.append(data);
		return true;
	}
	bool abort() override
	{
		return true;
	}
	bool validate(QNetworkReply &) override
	{
		if(m_expected.size() && m_expected != hash())
		{
			qWarning() << "Checksum mismatch, download is bad.";
			return false;
		}
		return true;
	}
	QByteArray hash()
	{
		return m_checksum.result();
	}
	void setExpected(QByteArray expected)
	{
		m_expected = expected;
	}

private: /* data */
	QByteArray data;
	QCryptographicHash m_checksum;
	QByteArray m_expected;
};
}