#pragma once

#include <QAbstractListModel>

#include "modplatform/modrinth/ModrinthPackManifest.h"
#include "ui/pages/modplatform/modrinth/ModrinthPage.h"

class ModPage;
class Version;

namespace Modrinth {

using LogoMap = QMap<QString, QIcon>;
using LogoCallback = std::function<void (QString)>;

class ModpackListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ModpackListModel(ModrinthPage* parent);
    ~ModpackListModel() override = default;

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
    void searchWithTerm(const QString& term, const int sort);

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    inline auto canFetchMore(const QModelIndex& parent) const -> bool override { return searchState == CanPossiblyFetchMore; };

   public slots:
    void searchRequestFinished(QJsonDocument& doc_all);
    void searchRequestFailed(QString reason);

    void versionRequestSucceeded(QJsonDocument doc, QString addonId);

   protected slots:

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void performPaginatedSearch();

   protected:
    void requestLogo(QString file, QString url);

    inline auto getMineVersions() const -> std::list<Version>;

   protected:
    ModrinthPage* m_parent;

    QList<Modrinth::Modpack> modpacks;

    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } searchState = None;

    NetJob::Ptr jobPtr;

    QByteArray m_all_response;
    QByteArray m_specific_response;
};
}  // namespace ModPlatform
