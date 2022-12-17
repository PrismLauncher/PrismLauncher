#pragma once

#include <RWStorage.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QIcon>
#include <QStyledItemDelegate>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

#include <functional>
#include <net/NetJob.h>

#include <modplatform/flame/FlamePackIndex.h>

namespace Flame {


typedef QMap<QString, QIcon> LogoMap;
typedef std::function<void(QString)> LogoCallback;

class ListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ListModel(QObject *parent);
    virtual ~ListModel();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool canFetchMore(const QModelIndex & parent) const override;
    void fetchMore(const QModelIndex & parent) override;

    void getLogo(const QString &logo, const QString &logoUrl, LogoCallback callback);
    void searchWithTerm(const QString & term, const int sort);

private slots:
    void performPaginatedSearch();

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void searchRequestFinished();
    void searchRequestFailed(QString reason);

private:
    void requestLogo(QString file, QString url);

private:
    QList<IndexedPack> modpacks;
    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failedLogos;
    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loadingLogos;
    LogoMap hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    QString currentSearchTerm;
    int currentSort = 0;
    int nextSearchOffset = 0;
    enum SearchState {
        None,
        CanPossiblyFetchMore,
        ResetRequested,
        Finished
    } searchState = None;
    NetJob::Ptr jobPtr;
    QByteArray response;
};

}
