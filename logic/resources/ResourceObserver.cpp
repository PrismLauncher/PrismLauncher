#include "ResourceObserver.h"

#include <QDebug>

#include "Resource.h"

static const char *defaultPropertyForTarget(QObject *target)
{
	if (target->inherits("QLabel"))
	{
		return "pixmap";
	}
	else if (target->inherits("QAction") ||
			 target->inherits("QMenu") ||
			 target->inherits("QAbstractButton"))
	{
		return "icon";
	}
	// for unit tests
	else if (target->inherits("DummyObserverObject"))
	{
		return "property";
	}
	else
	{
		Q_ASSERT_X(false, "ResourceObserver.cpp: defaultPropertyForTarget", "Unrecognized QObject subclass");
		return nullptr;
	}
}

QObjectResourceObserver::QObjectResourceObserver(QObject *target, const char *property)
	: QObject(target), m_target(target)
{
	const QMetaObject *mo = m_target->metaObject();
	m_property = mo->property(mo->indexOfProperty(
								  property ?
									  property
									: defaultPropertyForTarget(target)));
}
void QObjectResourceObserver::resourceUpdated()
{
	m_property.write(m_target, getInternal(m_property.type()));
}


ResourceObserver::~ResourceObserver()
{
	m_resource->notifyObserverDeleted(this);
}

QVariant ResourceObserver::getInternal(const int typeId) const
{
	Q_ASSERT(m_resource);
	return m_resource->getResourceInternal(typeId);
}
