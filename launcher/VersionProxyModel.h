#pragma once
#include <QAbstractProxyModel>
#include "BaseVersionList.h"

#include <Filter.h>

class VersionFilterModel;

class VersionProxyModel : public QAbstractProxyModel {
    Q_OBJECT
   public:
    enum Column { Name, ParentVersion, Branch, Type, CPUArchitecture, Path, Time, JavaName, JavaMajor };
    using FilterMap = QHash<BaseVersionList::ModelRoles, Filter>;

   public:
    VersionProxyModel(QObject* parent = nullptr);
    ~VersionProxyModel() override = default;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    const FilterMap& filters() const;
    const QString& search() const;
    void setFilter(BaseVersionList::ModelRoles column, Filter filter);
    void setSearch(const QString& search);
    void clearFilters();
    QModelIndex getRecommended() const;
    QModelIndex getVersion(const QString& version) const;
    void setCurrentVersion(const QString& version);
   private slots:

    void sourceDataChanged(const QModelIndex& source_top_left, const QModelIndex& source_bottom_right);

    void sourceAboutToBeReset();
    void sourceReset();

    void sourceRowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void sourceRowsInserted(const QModelIndex& parent, int first, int last);

    void sourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void sourceRowsRemoved(const QModelIndex& parent, int first, int last);

   private:
    QList<Column> m_columns;
    FilterMap m_filters;
    QString m_search;
    BaseVersionList::RoleList roles;
    VersionFilterModel* filterModel;
    bool hasRecommended = false;
    bool hasLatest = false;
    QString m_currentVersion;
};
