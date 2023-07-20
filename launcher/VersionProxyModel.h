#pragma once
#include <QAbstractProxyModel>
#include "BaseVersionList.h"

#include <Filter.h>

class VersionFilterModel;

class VersionProxyModel: public QAbstractProxyModel
{
    Q_OBJECT
public:

    enum Column
    {
        Name,
        ParentVersion,
        Branch,
        Type,
        Architecture,
        Path,
        Time
    };
    typedef QHash<BaseVersionList::ModelRoles, std::shared_ptr<Filter>> FilterMap;

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
    const QString &search() const;
    void setFilter(const BaseVersionList::ModelRoles column, Filter * filter);
    void setSearch(const QString &search);
    void clearFilters();
    QModelIndex getRecommended() const;
    QModelIndex getVersion(const QString & version) const;
    void setCurrentVersion(const QString &version);
private slots:

    void sourceDataChanged(const QModelIndex &source_top_left,const QModelIndex &source_bottom_right);

    void sourceAboutToBeReset();
    void sourceReset();

    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void sourceRowsInserted(const QModelIndex &parent, int first, int last);

    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void sourceRowsRemoved(const QModelIndex &parent, int first, int last);

private:
    QList<Column> m_columns;
    FilterMap m_filters;
    QString m_search;
    BaseVersionList::RoleList roles;
    VersionFilterModel * filterModel;
    bool hasRecommended = false;
    bool hasLatest = false;
    QString m_currentVersion;
};
