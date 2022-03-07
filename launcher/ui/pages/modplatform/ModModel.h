#pragma once

#include <QAbstractListModel>

#include "modplatform/ModAPI.h"
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
    virtual ~ListModel() = default;

    inline int rowCount(const QModelIndex& parent) const override { return modpacks.size(); };
    inline int columnCount(const QModelIndex& parent) const override { return 1; };
    inline Qt::ItemFlags flags(const QModelIndex& index) const override { return QAbstractListModel::flags(index); };

    QString debugName() const;

    /* Retrieve information from the model at a given index with the given role */
    QVariant data(const QModelIndex& index, int role) const override;

    inline void setActiveJob(NetJob::Ptr ptr) { jobPtr = ptr; }

    /* Ask the API for more information */
    void fetchMore(const QModelIndex& parent) override;
    void searchWithTerm(const QString& term, const int sort);
    void requestModVersions(const ModPlatform::IndexedPack& current);

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    inline bool canFetchMore(const QModelIndex& parent) const override { return searchState == CanPossiblyFetchMore; };

   public slots:
    void searchRequestFinished(QJsonDocument& doc);
    void searchRequestFailed(QString reason);

    void versionRequestSucceeded(QJsonDocument doc, QString addonId);

   protected slots:

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void performPaginatedSearch();

   protected:
    virtual void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual QJsonArray documentToArray(QJsonDocument& obj) const = 0;
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
};
}  // namespace ModPlatform
