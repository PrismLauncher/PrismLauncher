#pragma once
#include <QAbstractProxyModel>
#include "BaseVersionList.h"

class VersionFilterModel;

class VersionProxyModel: public QAbstractProxyModel
{
	Q_OBJECT
public:
	struct Filter
	{
		QString string;
		bool exact = false;
	};
	enum Column
	{
		Name,
		ParentVersion,
		Branch,
		Type,
		Architecture,
		Path
	};
	typedef QHash<BaseVersionList::ModelRoles, Filter> FilterMap;

public:
    VersionProxyModel ( QObject* parent = 0 );
    virtual ~VersionProxyModel() {};

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
	virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	virtual QModelIndex parent(const QModelIndex &child) const override;
	virtual void setSourceModel(QAbstractItemModel *sourceModel) override;

	const FilterMap &filters() const;
	void setFilter(const BaseVersionList::ModelRoles column, const QString &filter, const bool exact);
	void clearFilters();
	QModelIndex getRecommended() const;
private slots:

	void sourceDataChanged(const QModelIndex &source_top_left,const QModelIndex &source_bottom_right);
	void sourceAboutToBeReset();
	void sourceReset();

private:
	QList<Column> m_columns;
	FilterMap m_filters;
	BaseVersionList::RoleList roles;
	VersionFilterModel * filterModel;
	bool hasRecommended = false;
	bool hasLatest = false;
};
