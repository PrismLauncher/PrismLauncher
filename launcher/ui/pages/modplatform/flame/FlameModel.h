#pragma once

#include <RWStorage.h>

#include <QAbstractListModel>
#include <QIcon>
#include <QList>
#include <QMetaType>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QThreadPool>

#include <net/NetJob.h>
#include <functional>
#include "ui/widgets/ModFilterWidget.h"

namespace Flame {

using LogoMap = QMap<QString, QIcon>;
using LogoCallback = std::function<void(QString)>;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(QObject* parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);
    void searchWithTerm(const QString& term, int sort, std::shared_ptr<ModFilterWidget::Filter> filter, bool filterChanged);

    bool hasActiveSearchJob() const { return m_jobPtr && m_jobPtr->isRunning(); }
    Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? m_jobPtr : nullptr; }

   private slots:
    void performPaginatedSearch();

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void searchRequestFinished(QList<ModPlatform::IndexedPack::Ptr>&);
    void searchRequestFailed(QString reason);
    void searchRequestForOneSucceeded(ModPlatform::IndexedPack::Ptr);

   private:
    void requestLogo(QString file, QString url);

   private:
    QList<ModPlatform::IndexedPack::Ptr> m_modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> m_waitingCallbacks;

    QString m_currentSearchTerm;
    int m_currentSort = 0;
    std::shared_ptr<ModFilterWidget::Filter> m_filter;
    int m_nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } m_searchState = None;
    Task::Ptr m_jobPtr;
};

}  // namespace Flame
