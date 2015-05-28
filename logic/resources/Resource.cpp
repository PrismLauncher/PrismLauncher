#include "Resource.h"

#include <QDebug>

#include "WebResourceHandler.h"
#include "IconResourceHandler.h"
#include "ResourceObserver.h"

QMap<QString, std::function<std::shared_ptr<ResourceHandler>(const QString &)>> Resource::m_handlers;
QMap<QPair<int, int>, std::function<QVariant(QVariant)>> Resource::m_transfomers;
QMap<QString, std::weak_ptr<Resource>> Resource::m_resources;

Resource::Resource(const QString &resource)
{
	if (!m_handlers.contains("web"))
	{
		registerHandler<WebResourceHandler>("web");
	}
	if (!m_handlers.contains("icon"))
	{
		registerHandler<IconResourceHandler>("icon");
	}

	Q_ASSERT(resource.contains(':'));
	const QString resourceId = resource.left(resource.indexOf(':'));
	Q_ASSERT(m_handlers.contains(resourceId));
	m_handler = m_handlers.value(resourceId)(resource.mid(resource.indexOf(':') + 1));
	m_handler->init(m_handler);
	m_handler->setResource(this);
	Q_ASSERT(m_handler);
}
Resource::~Resource()
{
	qDeleteAll(m_observers);
}

Resource::Ptr Resource::create(const QString &resource)
{
	Resource::Ptr ptr = m_resources.contains(resource)
			? m_resources.value(resource).lock()
			: nullptr;
	if (!ptr)
	{
		struct ConstructableResource : public Resource
		{
			explicit ConstructableResource(const QString &resource)
				: Resource(resource) {}
		};
		ptr = std::make_shared<ConstructableResource>(resource);
		m_resources.insert(resource, ptr);
	}
	return ptr;
}

Resource::Ptr Resource::applyTo(ResourceObserver *observer)
{
	m_observers.append(observer);
	observer->setSource(shared_from_this()); // give the observer a shared_ptr for us so we don't get deleted
	observer->resourceUpdated();
	return shared_from_this();
}
Resource::Ptr Resource::applyTo(QObject *target, const char *property)
{
	// the cast to ResourceObserver* is required to ensure the right overload gets choosen
	return applyTo(static_cast<ResourceObserver *>(new QObjectResourceObserver(target, property)));
}

Resource::Ptr Resource::placeholder(Resource::Ptr other)
{
	m_placeholder = other;
	for (ResourceObserver *observer : m_observers)
	{
		observer->resourceUpdated();
	}
	return shared_from_this();
}

QVariant Resource::getResourceInternal(const int typeId) const
{
	if (m_handler->result().isNull() && m_placeholder)
	{
		return m_placeholder->getResourceInternal(typeId);
	}
	const QVariant variant = m_handler->result();
	const auto typePair = qMakePair(int(variant.type()), typeId);
	if (m_transfomers.contains(typePair))
	{
		return m_transfomers.value(typePair)(variant);
	}
	else
	{
		return variant;
	}
}

void Resource::reportResult()
{
	for (ResourceObserver *observer : m_observers)
	{
		observer->resourceUpdated();
	}
}
void Resource::reportFailure(const QString &reason)
{
	for (ResourceObserver *observer : m_observers)
	{
		observer->setFailure(reason);
	}
}
void Resource::reportProgress(const int progress)
{
	for (ResourceObserver *observer : m_observers)
	{
		observer->setProgress(progress);
	}
}

void Resource::notifyObserverDeleted(ResourceObserver *observer)
{
	m_observers.removeAll(observer);
}
