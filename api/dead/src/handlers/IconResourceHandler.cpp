#include "IconResourceHandler.h"
#include <xdgicon.h>

#include <QDir>
#include <QDebug>

QList<std::weak_ptr<IconResourceHandler>> IconResourceHandler::m_iconHandlers;

IconResourceHandler::IconResourceHandler(const QString &key)
	: m_key(key)
{
}

void IconResourceHandler::setTheme(const QString &theme)
{
	// notify everyone
	for (auto handler : m_iconHandlers)
	{
		std::shared_ptr<IconResourceHandler> ptr = handler.lock();
		if (ptr)
		{
			ptr->setResult(ptr->get());
		}
	}
}

void IconResourceHandler::init(std::shared_ptr<ResourceHandler> &ptr)
{
	m_iconHandlers.append(std::dynamic_pointer_cast<IconResourceHandler>(ptr));
	// we always have a result, so lets report it now!
	setResult(get());
}

QVariant IconResourceHandler::get() const
{
	return XdgIcon::fromTheme(m_key);
}
