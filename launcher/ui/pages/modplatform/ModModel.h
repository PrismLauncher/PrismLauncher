#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "net/NetJob.h"

class ModPage;
class Version;

namespace ModPlatform {

using LogoMap = QMap<QString, QIcon>;
using LogoCallback = std::function<void (QString)>;

class ListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ListModel(ModPage* parent);
    ~ListModel() override = default;

    inline auto rowCount(const QModelIndex& parent) const -> int override { return modpacks.size(); };
    inline auto columnCount(const QModelIndex& parent) const -> int override { return 1; };
    inline auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); };

    auto debugName() const -> QString;

    /* Retrieve information from the model at a given index with the given role */
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    inline void setActiveJob(NetJob::Ptr ptr) { jobPtr = ptr; }

    /* Ask the API for more information */
    void fetchMore(const QModelIndex& parent) override;
    void refresh();
    void searchWithTerm(const QString& term, const int sort, const bool filter_changed);
    void requestModInfo(ModPlatform::IndexedPack& current);
    void requestModVersions(const ModPlatform::IndexedPack& current);

    virtual void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) {};
    virtual void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) = 0;

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    inline auto canFetchMore(const QModelIndex& parent) const -> bool override { return searchState == CanPossiblyFetchMore; };

   public slots:
    void searchRequestFinished(QJsonDocument& doc);
    void searchRequestFailed(QString reason);

    void infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack);

    void versionRequestSucceeded(QJsonDocument doc, QString addonId);

   protected slots:

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void performPaginatedSearch();

   protected:
    virtual auto documentToArray(QJsonDocument& obj) const -> QJsonArray = 0;
    virtual auto getSorts() const -> const char** = 0;

    void requestLogo(QString file, QString url);

    inline auto getMineVersions() const -> std::list<Version>;

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
