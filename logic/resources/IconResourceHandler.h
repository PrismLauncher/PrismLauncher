#pragma once

#include <memory>

#include "ResourceHandler.h"

class IconResourceHandler : public ResourceHandler
{
public:
	explicit IconResourceHandler(const QString &key);

	static void setTheme(const QString &theme);

private:
	void init(std::shared_ptr<ResourceHandler> &ptr) override;

	QString m_key;
	static QString m_theme;
	static QList<std::weak_ptr<IconResourceHandler>> m_iconHandlers;

	QVariant get() const;
};
