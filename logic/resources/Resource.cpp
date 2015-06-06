#include "Resource.h"

#include <QDebug>

#include "WebResourceHandler.h"
#include "IconResourceHandler.h"
#include "ResourceObserver.h"

// definition of static members of Resource
QMap<QString, std::function<std::shared_ptr<ResourceHandler>(const QString &)>> Resource::m_handlers;
QMap<QPair<int, int>, std::function<QVariant(QVariant)>> Resource::m_transfomers;
QMap<QString, std::weak_ptr<Resource>> Resource::m_resources;

Resource::Resource(const QString &resource)
	: m_resource(resource)
{
	// register default handlers
	// QUESTION: move elsewhere?
	if (!m_handlers.contains("web"))
	{
		registerHandler<WebResourceHandler>("web");
	}
	if (!m_handlers.contains("icon"))
	{
		registerHandler<IconResourceHandler>("icon");
	}

	// a valid resource identifier has the format <id>:<data>
	Q_ASSERT(resource.contains(':'));
	// "parse" the resource identifier into id and data
	const QString resourceId = resource.left(resource.indexOf(':'));
	const QString resourceData = resource.mid(resource.indexOf(':') + 1);

	// create and set up the handler
	Q_ASSERT(m_handlers.contains(resourceId));
	m_handler = m_handlers.value(resourceId)(resourceData);
	Q_ASSERT(m_handler);
	m_handler->init(m_handler);
	m_handler->setResource(this);
}
Resource::~Resource()
{
	qDeleteAll(m_observers);
}

Resource::Ptr Resource::create(const QString &resource, Ptr placeholder)
{
	const QString storageId = storageIdentifier(resource, placeholder);

	// do we already have a resource? even if m_resources contains it it might not be valid any longer (weak_ptr)
	Resource::Ptr ptr = m_resources.contains(storageId)
			? m_resources.value(storageId).lock()
			: nullptr;
	// did we have one? and is it still valid?
	if (!ptr)
	{
		/* We don't want Resource to have a public constructor, but std::make_shared needs it,
		 * so we create a subclass of Resource here that exposes the constructor as public.
		 * The alternative would be making the allocator for std::make_shared a friend, but it
		 * differs between different STL implementations, so that would be a pain.
		 */
		struct ConstructableResource : public Resource
		{
			explicit ConstructableResource(const QString &resource)
				: Resource(resource) {}
		};
		ptr = std::make_shared<ConstructableResource>(resource);
		ptr->m_placeholder = placeholder;
		m_resources.insert(storageId, ptr);
	}
	return ptr;
}

Resource::Ptr Resource::applyTo(ResourceObserver *observer)
{
	m_observers.append(observer);
	observer->setSource(shared_from_this()); // give the observer a shared_ptr for us so we don't get deleted
	observer->resourceUpdated(); // ask the observer to poll us immediently, we might already have data
	return shared_from_this(); // allow chaining
}
Resource::Ptr Resource::applyTo(QObject *target, const char *property)
{
	// the cast to ResourceObserver* is required to ensure the right overload gets choosen,
	// since QObjectResourceObserver also inherits from QObject
	return applyTo(static_cast<ResourceObserver *>(new QObjectResourceObserver(target, property)));
}

QVariant Resource::getResourceInternal(const int typeId) const
{
	// no result (yet), but a placeholder? delegate to the placeholder.
	if (m_handler->result().isNull() && m_placeholder)
	{
		return m_placeholder->getResourceInternal(typeId);
	}
	const QVariant variant = m_handler->result();
	const auto typePair = qMakePair(int(variant.type()), typeId);

	// do we have an explicit transformer? use it.
	if (m_transfomers.contains(typePair))
	{
		return m_transfomers.value(typePair)(variant);
	}
	else
	{
		// we do not have an explicit transformer, so we just pass the QVariant, which will automatically
		// transform some types for us (different numbers to each other etc.)
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

QString Resource::storageIdentifier(const QString &id, Resource::Ptr placeholder)
{
	if (placeholder)
	{
		return id + '#' + storageIdentifier(placeholder->m_resource, placeholder->m_placeholder);
	}
	else
	{
		return id;
	}
}
