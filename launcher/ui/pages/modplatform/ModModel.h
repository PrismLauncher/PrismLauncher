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
    ~ListModel() override;

    inline auto rowCount(const QModelIndex& parent) const -> int override { return parent.isValid() ? 0 : modpacks.size(); };
    inline auto columnCount(const QModelIndex& parent) const -> int override { return parent.isValid() ? 0 : 1; };
    inline auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); };

    auto debugName() const -> QString;

    /* Retrieve information from the model at a given index with the given role */
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    inline void setActiveJob(NetJob::Ptr ptr) { jobPtr = ptr; }
    inline NetJob* activeJob() { return jobPtr.get(); }

    /* Ask the API for more information */
    void fetchMore(const QModelIndex& parent) override;
    void refresh();
    void searchWithTerm(const QString& term, const int sort, const bool filter_changed);
    void requestModInfo(ModPlatform::IndexedPack& current, QModelIndex index);
    void requestModVersions(const ModPlatform::IndexedPack& current, QModelIndex index);

    virtual void loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj) = 0;
    virtual void loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr) = 0;

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    inline auto canFetchMore(const QModelIndex& parent) const -> bool override { return parent.isValid() ? false : searchState == CanPossiblyFetchMore; };

   public slots:
    void searchRequestFinished(QJsonDocument& doc);
    void searchRequestFailed(QString reason);
    void searchRequestAborted();

    void infoRequestFinished(QJsonDocument& doc, ModPlatform::IndexedPack& pack, const QModelIndex& index);

    void versionRequestSucceeded(QJsonDocument doc, QString addonId, const QModelIndex& index);

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
    ModPage* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent;

    QList<ModPlatform::IndexedPack> modpacks;

    LogoMap hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;
    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failedLogos;
    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loadingLogos;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } searchState = None;

    NetJob::Ptr jobPtr;
};
}  // namespace ModPlatform
