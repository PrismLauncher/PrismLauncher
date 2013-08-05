#pragma once

#include "BaseInstance_p.h"
#include "OneSixVersion.h"

struct OneSixInstancePrivate: public BaseInstancePrivate
{
	QSharedPointer<FullVersion> version;
};