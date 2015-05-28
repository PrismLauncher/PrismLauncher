#pragma once

#include <QIdentityProxyModel>
#include <memory>

/// Convenience proxy model that transforms resource identifiers (strings) for Qt::DecorationRole into other types.
class ResourceProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	// resultTypeId is found using qMetaTypeId<T>()
	explicit ResourceProxyModel(const int resultTypeId, QObject *parent = nullptr);

	enum
	{
		// provide this role from your model if you want to show a placeholder
		PlaceholderRole = Qt::UserRole + 0xabc // some random offset to not collide with other stuff
	};

	QVariant data(const QModelIndex &proxyIndex, int role) const override;
	void setSourceModel(QAbstractItemModel *model) override;

	template <typename T>
	static QAbstractItemModel *mixin(QAbstractItemModel *model)
	{
		ResourceProxyModel *proxy = new ResourceProxyModel(qMetaTypeId<T>(), model);
		proxy->setSourceModel(model);
		return proxy;
	}

private:
	// mutable because it needs to be available from the const data()
	mutable QMap<QPersistentModelIndex, std::shared_ptr<class Resource>> m_resources;

	const int m_resultTypeId;
};
