#include "ResourceProxyModel.h"

#include <QItemSelectionRange>

#include "Resource.h"
#include "ResourceObserver.h"

//Q_DECLARE_METATYPE(QVector<int>)

class ModelResourceObserver : public ResourceObserver
{
public:
	explicit ModelResourceObserver(const QModelIndex &index, const int role)
		: m_index(index), m_role(role)
	{
		qRegisterMetaType<QVector<int>>("QVector<int>");
	}

	void resourceUpdated() override
	{
		if (m_index.isValid())
		{
			QMetaObject::invokeMethod(const_cast<QAbstractItemModel *>(m_index.model()),
									  "dataChanged", Qt::QueuedConnection,
									  Q_ARG(QModelIndex, m_index), Q_ARG(QModelIndex, m_index), Q_ARG(QVector<int>, QVector<int>() << m_role));
		}
	}

private:
	QPersistentModelIndex m_index;
	int m_role;
};

ResourceProxyModel::ResourceProxyModel(const int resultTypeId, QObject *parent)
	: QIdentityProxyModel(parent), m_resultTypeId(resultTypeId)
{
}

QVariant ResourceProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
	const QModelIndex mapped = mapToSource(proxyIndex);
	if (mapped.isValid() && role == Qt::DecorationRole && !mapToSource(proxyIndex).data(role).toString().isEmpty())
	{
		if (!m_resources.contains(mapped))
		{
			Resource::Ptr res = Resource::create(mapToSource(proxyIndex).data(role).toString())
					->applyTo(new ModelResourceObserver(proxyIndex, role));

			const QVariant placeholder = mapped.data(PlaceholderRole);
			if (!placeholder.isNull() && placeholder.type() == QVariant::String)
			{
				res->placeholder(Resource::create(placeholder.toString()));
			}

			m_resources.insert(mapped, res);
		}

		return m_resources.value(mapped)->getResourceInternal(m_resultTypeId);
	}
	return mapped.data(role);
}

void ResourceProxyModel::setSourceModel(QAbstractItemModel *model)
{
	if (sourceModel())
	{
		disconnect(sourceModel(), 0, this, 0);
	}
	if (model)
	{
		connect(model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles)
		{
			if (roles.contains(Qt::DecorationRole) || roles.isEmpty())
			{
				const QItemSelectionRange range(tl, br);
				for (const QModelIndex &index : range.indexes())
				{
					m_resources.remove(index);
				}
			}
			else if (roles.contains(PlaceholderRole))
			{
				const QItemSelectionRange range(tl, br);
				for (const QModelIndex &index : range.indexes())
				{
					if (m_resources.contains(index))
					{
						const QVariant placeholder = index.data(PlaceholderRole);
						if (!placeholder.isNull() && placeholder.type() == QVariant::String)
						{
							m_resources.value(index)->placeholder(Resource::create(placeholder.toString()));
						}
						else
						{
							m_resources.value(index)->placeholder(nullptr);
						}
					}
				}
			}
		});
	}
	QIdentityProxyModel::setSourceModel(model);
}
