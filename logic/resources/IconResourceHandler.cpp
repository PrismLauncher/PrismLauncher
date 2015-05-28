#include "IconResourceHandler.h"

#include <QDir>
#include <QDebug>

QString IconResourceHandler::m_theme = "multimc";
QList<std::weak_ptr<IconResourceHandler>> IconResourceHandler::m_iconHandlers;

IconResourceHandler::IconResourceHandler(const QString &key)
	: m_key(key)
{
}

void IconResourceHandler::setTheme(const QString &theme)
{
	m_theme = theme;

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
	setResult(get());
}

QVariant IconResourceHandler::get() const
{
	const QDir iconsDir = QDir(":/icons/" + m_theme);

	QVariantMap out;
	for (const QFileInfo &sizeInfo : iconsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		const QDir dir = QDir(sizeInfo.absoluteFilePath());
		const QString dirName = sizeInfo.fileName();
		const int size = dirName.left(dirName.indexOf('x')).toInt();
		if (dir.exists(m_key + ".png") && dirName != "scalable")
		{
			out.insert(dir.absoluteFilePath(m_key + ".png"), size);
		}
		else if (dir.exists(m_key + ".svg") && dirName == "scalable")
		{
			out.insert(dir.absoluteFilePath(m_key + ".svg"), size);
		}
	}

	if (out.isEmpty())
	{
		qWarning() << "Couldn't find any icons for" << m_key;
	}

	return out;
}
