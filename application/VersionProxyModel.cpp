#include "VersionProxyModel.h"
#include "MultiMC.h"
#include <QSortFilterProxyModel>
#include <QPixmapCache>
#include <Version.h>

class VersionFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	VersionFilterModel(VersionProxyModel *parent) : QSortFilterProxyModel(parent)
	{
		m_parent = parent;
		setSortRole(BaseVersionList::SortRole);
		sort(0, Qt::DescendingOrder);
	}

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		const auto &filters = m_parent->filters();
		for (auto it = filters.begin(); it != filters.end(); ++it)
		{
			auto role = it.key();
			auto idx = sourceModel()->index(source_row, 0, source_parent);
			auto data = sourceModel()->data(idx, role);

			switch(role)
			{
				case BaseVersionList::ParentGameVersionRole:
				case BaseVersionList::VersionIdRole:
				{
					auto versionString = data.toString();
					if(it.value().exact)
					{
						return versionString == it.value().string;
					}
					else
					{
						return versionIsInInterval(versionString, it.value().string);
					}
				}
				default:
				{
					auto match = data.toString();
					if(it.value().exact)
					{
						if (match != it.value().string)
						{
							return false;
						}
					}
					else if (match.contains(it.value().string))
					{
						return false;
					}
				}
			}
		}
		return true;
	}
private:
	VersionProxyModel *m_parent;
};

VersionProxyModel::VersionProxyModel(QObject *parent) : QAbstractProxyModel(parent)
{
	filterModel = new VersionFilterModel(this);
	connect(filterModel, &QAbstractItemModel::dataChanged, this, &VersionProxyModel::sourceDataChanged);
	// FIXME: implement when needed
	/*
	connect(replacing, &QAbstractItemModel::rowsAboutToBeInserted, this, &VersionProxyModel::sourceRowsAboutToBeInserted);
	connect(replacing, &QAbstractItemModel::rowsInserted, this, &VersionProxyModel::sourceRowsInserted);
	connect(replacing, &QAbstractItemModel::rowsAboutToBeRemoved, this, &VersionProxyModel::sourceRowsAboutToBeRemoved);
	connect(replacing, &QAbstractItemModel::rowsRemoved, this, &VersionProxyModel::sourceRowsRemoved);
	connect(replacing, &QAbstractItemModel::rowsAboutToBeMoved, this, &VersionProxyModel::sourceRowsAboutToBeMoved);
	connect(replacing, &QAbstractItemModel::rowsMoved, this, &VersionProxyModel::sourceRowsMoved);
	connect(replacing, &QAbstractItemModel::layoutAboutToBeChanged, this, &VersionProxyModel::sourceLayoutAboutToBeChanged);
	connect(replacing, &QAbstractItemModel::layoutChanged, this, &VersionProxyModel::sourceLayoutChanged);
	*/
	connect(filterModel, &QAbstractItemModel::modelAboutToBeReset, this, &VersionProxyModel::sourceAboutToBeReset);
	connect(filterModel, &QAbstractItemModel::modelReset, this, &VersionProxyModel::sourceReset);

	QAbstractProxyModel::setSourceModel(filterModel);
}

QVariant VersionProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(section < 0 || section >= m_columns.size())
		return QVariant();
	if(orientation != Qt::Horizontal)
		return QVariant();
	auto column = m_columns[section];
	if(role == Qt::DisplayRole)
	{
		switch(column)
		{
			case Name:
				return tr("Version");
			case ParentVersion:
				return tr("Minecraft"); //FIXME: this should come from metadata
			case Branch:
				return tr("Branch");
			case Type:
				return tr("Type");
			case Architecture:
				return tr("Architecture");
			case Path:
				return tr("Path");
		}
	}
	else if(role == Qt::ToolTipRole)
	{
		switch(column)
		{
			case Name:
				return tr("The name of the version.");
			case ParentVersion:
				return tr("Minecraft version"); //FIXME: this should come from metadata
			case Branch:
				return tr("The version's branch");
			case Type:
				return tr("The version's type");
			case Architecture:
				return tr("CPU Architecture");
			case Path:
				return tr("Filesystem path to this version");
		}
	}
	return QVariant();
}

QVariant VersionProxyModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
	{
		return QVariant();
	}
	auto column = m_columns[index.column()];
	auto parentIndex = mapToSource(index);
	switch(role)
	{
		case Qt::DisplayRole:
		{
			switch(column)
			{
				case Name:
					return sourceModel()->data(parentIndex, BaseVersionList::VersionRole);
				case ParentVersion:
					return sourceModel()->data(parentIndex, BaseVersionList::ParentGameVersionRole);
				case Branch:
					return sourceModel()->data(parentIndex, BaseVersionList::BranchRole);
				case Type:
					return sourceModel()->data(parentIndex, BaseVersionList::TypeRole);
				case Architecture:
					return sourceModel()->data(parentIndex, BaseVersionList::ArchitectureRole);
				case Path:
					return sourceModel()->data(parentIndex, BaseVersionList::PathRole);
				default:
					return QVariant();
			}
		}
		case Qt::ToolTipRole:
		{
			switch(column)
			{
				case Name:
				{
					if(hasRecommended)
					{
						auto value = sourceModel()->data(parentIndex, BaseVersionList::RecommendedRole);
						if(value.toBool())
						{
							return tr("Recommended");
						}
						else if(hasLatest)
						{
							auto value = sourceModel()->data(parentIndex, BaseVersionList::LatestRole);
							if(value.toBool())
							{
								return tr("Latest");
							}
						}
						else if(index.row() == 0)
						{
							return tr("Latest");
						}
					}
				}
				default:
				{
					return sourceModel()->data(parentIndex, BaseVersionList::VersionIdRole);
				}
			}
		}
		case Qt::DecorationRole:
		{
			switch(column)
			{
				case Name:
				{
					if(hasRecommended)
					{
						auto value = sourceModel()->data(parentIndex, BaseVersionList::RecommendedRole);
						if(value.toBool())
						{
							return MMC->getThemedIcon("star");
						}
						else if(hasLatest)
						{
							auto value = sourceModel()->data(parentIndex, BaseVersionList::LatestRole);
							if(value.toBool())
							{
								return MMC->getThemedIcon("bug");
							}
						}
						else if(index.row() == 0)
						{
							return MMC->getThemedIcon("bug");
						}
						auto pixmap = QPixmapCache::find("placeholder");
						if(!pixmap)
						{
							QPixmap px(16,16);
							px.fill(Qt::transparent);
							QPixmapCache::insert("placeholder", px);
							return px;
						}
						return *pixmap;
					}
				}
				default:
				{
					return QVariant();
				}
			}
		}
		default:
		{
			if(roles.contains((BaseVersionList::ModelRoles)role))
			{
				return sourceModel()->data(parentIndex, role);
			}
			return QVariant();
		}
	}
};

QModelIndex VersionProxyModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

QModelIndex VersionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
	if(sourceIndex.isValid())
	{
		return index(sourceIndex.row(), 0);
	}
	return QModelIndex();
}

QModelIndex VersionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
	if(proxyIndex.isValid())
	{
		return sourceModel()->index(proxyIndex.row(), 0);
	}
	return QModelIndex();
}

QModelIndex VersionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
	// no trees here... shoo
	if(parent.isValid())
	{
		return QModelIndex();
	}
	if(row < 0 || row >= sourceModel()->rowCount())
		return QModelIndex();
	if(column < 0 || column >= columnCount())
		return QModelIndex();
	return QAbstractItemModel::createIndex(row, column);
}

int VersionProxyModel::columnCount(const QModelIndex &parent) const
{
	return m_columns.size();
}

int VersionProxyModel::rowCount(const QModelIndex &parent) const
{
	if(sourceModel())
	{
		return sourceModel()->rowCount();
	}
	return 0;
}

void VersionProxyModel::sourceDataChanged(const QModelIndex &source_top_left,
										  const QModelIndex &source_bottom_right)
{
	if(source_top_left.parent() != source_bottom_right.parent())
		return;

	// whole row is getting changed
	auto topLeft = createIndex(source_top_left.row(), 0);
	auto bottomRight = createIndex(source_bottom_right.row(), columnCount() - 1);
	emit dataChanged(topLeft, bottomRight);
}

void VersionProxyModel::setSourceModel(QAbstractItemModel *replacingRaw)
{
	auto replacing = dynamic_cast<BaseVersionList *>(replacingRaw);
	beginResetModel();

	if(!replacing)
	{
		m_columns.clear();
		roles.clear();
		filterModel->setSourceModel(replacing);
		return;
	}

	roles = replacing->providesRoles();
	if(roles.contains(BaseVersionList::VersionRole))
	{
		m_columns.push_back(Name);
	}
	/*
	if(roles.contains(BaseVersionList::ParentGameVersionRole))
	{
		m_columns.push_back(ParentVersion);
	}
	*/
	if(roles.contains(BaseVersionList::ArchitectureRole))
	{
		m_columns.push_back(Architecture);
	}
	if(roles.contains(BaseVersionList::PathRole))
	{
		m_columns.push_back(Path);
	}
	if(roles.contains(BaseVersionList::BranchRole))
	{
		m_columns.push_back(Branch);
	}
	if(roles.contains(BaseVersionList::TypeRole))
	{
		m_columns.push_back(Type);
	}
	if(roles.contains(BaseVersionList::RecommendedRole))
	{
		hasRecommended = true;
	}
	if(roles.contains(BaseVersionList::LatestRole))
	{
		hasLatest = true;
	}
	filterModel->setSourceModel(replacing);

	endResetModel();
}

QModelIndex VersionProxyModel::getRecommended() const
{
	if(!roles.contains(BaseVersionList::RecommendedRole))
	{
		return index(0, 0);
	}
	int recommended = 0;
	for (int i = 0; i < rowCount(); i++)
	{
		auto value = sourceModel()->data(mapToSource(index(i, 0)), BaseVersionList::RecommendedRole);
		if (value.toBool())
		{
			recommended = i;
		}
	}
	return index(recommended, 0);
}

void VersionProxyModel::clearFilters()
{
	m_filters.clear();
	filterModel->invalidate();
}

void VersionProxyModel::setFilter(const BaseVersionList::ModelRoles column, const QString &filter, const bool exact)
{
	Filter f;
	f.string = filter;
	f.exact = exact;
	m_filters[column] = f;
	filterModel->invalidate();
}

const VersionProxyModel::FilterMap &VersionProxyModel::filters() const
{
	return m_filters;
}

void VersionProxyModel::sourceAboutToBeReset()
{
	beginResetModel();
}

void VersionProxyModel::sourceReset()
{
	endResetModel();
}

#include "VersionProxyModel.moc"
