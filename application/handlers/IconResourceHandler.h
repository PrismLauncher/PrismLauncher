#pragma once

#include <memory>
#include <resources/ResourceHandler.h>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT IconResourceHandler : public ResourceHandler
{
public:
	explicit IconResourceHandler(const QString &key);

	/// Sets the current theme and notifies all IconResourceHandlers of the change
	static void setTheme(const QString &theme);

private:
	// we need to keep track of all IconResourceHandlers so that we can update them if the theme changes
	void init(std::shared_ptr<ResourceHandler> &ptr) override;
	static QList<std::weak_ptr<IconResourceHandler>> m_iconHandlers;

	QString m_key;

	// the workhorse, returns QVariantMap (filename => size) for m_key/m_theme
	QVariant get() const;
};
