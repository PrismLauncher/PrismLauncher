#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "modplatform/ModAPI.h"
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

    virtual void requestModVersions(const ModPlatform::IndexedPack& current);

   protected slots:
    virtual void searchRequestFinished() = 0;
    void searchRequestFailed(QString reason);

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void performPaginatedSearch();

   protected:
    virtual const char** getSorts() const = 0;

    void requestLogo(QString file, QString url);

   protected:
    ModPage* m_parent;

    QList<ModPlatform::IndexedPack> modpacks;

    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } searchState = None;

    NetJob::Ptr jobPtr;
    QByteArray response;
};
}  // namespace ModPlatform
