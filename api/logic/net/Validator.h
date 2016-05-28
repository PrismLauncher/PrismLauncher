#pragma once

#include "net/NetAction.h"

#include "multimc_logic_export.h"

namespace Net {
class MULTIMC_LOGIC_EXPORT Validator
{
public: /* con/des */
	Validator() {};
	virtual ~Validator() {};

public: /* methods */
	virtual bool init(QNetworkRequest & request) = 0;
	virtual bool write(QByteArray & data) = 0;
	virtual bool abort() = 0;
	virtual bool validate(QNetworkReply & reply) = 0;
};
}