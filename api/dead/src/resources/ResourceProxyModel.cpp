#include "ResourceProxyModel.h"

#include <QItemSelectionRange>

#include "Resource.h"
#include "ResourceObserver.h"

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
			// the resource changed, pretend to be the model and notify the views of the update. they will re-poll the model which will return the new resource value
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
	// valid cell that's a Qt::DecorationRole and that contains a non-empty string
	if (mapped.isValid() && role == Qt::DecorationRole && !mapToSource(proxyIndex).data(role).toString().isEmpty())
	{
		// do we already have a resource for this index?
		if (!m_resources.contains(mapped))
		{
			Resource::Ptr placeholder;
			const QVariant placeholderIdentifier = mapped.data(PlaceholderRole);
			if (!placeholderIdentifier.isNull() && placeholderIdentifier.type() == QVariant::String)
			{
				placeholder = Resource::create(placeholderIdentifier.toString());
			}

			// create the Resource and apply the observer for models
			Resource::Ptr res = Resource::create(mapToSource(proxyIndex).data(role).toString(), placeholder)
					->applyTo(new ModelResourceObserver(proxyIndex, role));

			m_resources.insert(mapped, res);
		}

		return m_resources.value(mapped)->getResourceInternal(m_resultTypeId);
	}
	// otherwise fall back to the source model
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
			// invalidate resources so that they will be re-created
			if (roles.contains(Qt::DecorationRole) || roles.contains(PlaceholderRole) || roles.isEmpty())
			{
				const QItemSelectionRange range(tl, br);
				for (const QModelIndex &index : range.indexes())
				{
					m_resources.remove(index);
				}
			}
		});
	}
	QIdentityProxyModel::setSourceModel(model);
}
