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

#include <modplatform/flame/FlamePackIndex.h>

namespace Flame {

typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

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
    void searchWithTerm(const QString& term, const int sort);

    [[nodiscard]] bool hasActiveSearchJob() const { return jobPtr && jobPtr->isRunning(); }
    [[nodiscard]] Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? jobPtr : nullptr; }

   private slots:
    void performPaginatedSearch();

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void searchRequestFinished();
    void searchRequestFailed(QString reason);
    void searchRequestForOneSucceeded(QJsonDocument&);

   private:
    void requestLogo(QString file, QString url);

   private:
    QList<IndexedPack> modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    LogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } searchState = None;
    Task::Ptr jobPtr;
    std::shared_ptr<QByteArray> response = std::make_shared<QByteArray>();
};

}  // namespace Flame
