#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "net/NetJob.h"

class ModPage;

namespace ModPlatform {

typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(ModPage* parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);
    void searchWithTerm(const QString& term, const int sort);

   protected slots:
    virtual void performPaginatedSearch() = 0;
    virtual void searchRequestFinished() = 0;

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void searchRequestFailed(QString reason);

   protected:
    void requestLogo(QString file, QString url);

   protected:
    ModPage* m_parent;

    QList<ModPlatform::IndexedPack> modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } searchState = None;
    NetJob::Ptr jobPtr;
    QByteArray response;
};
}  // namespace ModPlatform
