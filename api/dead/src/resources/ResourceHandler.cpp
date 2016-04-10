#include "ResourceHandler.h"

#include "Resource.h"

void ResourceHandler::setResult(const QVariant &result)
{
	m_result = result;
	if (m_resource)
	{
		m_resource->reportResult();
	}
}

void ResourceHandler::setFailure(const QString &reason)
{
	if (m_resource)
	{
		m_resource->reportFailure(reason);
	}
}

void ResourceHandler::setProgress(const int progress)
{
	if (m_resource)
	{
		m_resource->reportProgress(progress);
	}
}
