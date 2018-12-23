#pragma once

#include <modplatform/ftb/PackHelpers.h>
#include <RWStorage.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QIcon>
#include <QStyledItemDelegate>

#include <functional>

typedef QMap<QString, QIcon> FtbLogoMap;
typedef std::function<void(QString)> LogoCallback;

class FtbFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FtbFilterModel(QObject* parent = Q_NULLPTR);
    enum Sorting {
        ByName,
        ByGameVersion
    };
    const QMap<QString, Sorting> getAvailableSortings();
    QString translateCurrentSorting();
    void setSorting(Sorting sorting);
    Sorting getCurrentSorting();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QMap<QString, Sorting> sortings;
    Sorting currentSorting;

};

class FtbListModel : public QAbstractListModel
{
    Q_OBJECT
private:
    FtbModpackList modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    FtbLogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    void requestLogo(QString file);
    QString translatePackType(FtbPackType type) const;


private slots:
    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

public:
    FtbListModel(QObject *parent);
    ~FtbListModel();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void fill(FtbModpackList modpacks);
    void addPack(FtbModpack modpack);
    void clear();
    void remove(int row);

    FtbModpack at(int row);
    void getLogo(const QString &logo, LogoCallback callback);
};
