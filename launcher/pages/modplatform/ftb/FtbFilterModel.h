#pragma once

#include <QtCore/QSortFilterProxyModel>

namespace Ftb {

class FilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilterModel(QObject* parent = Q_NULLPTR);
    enum Sorting {
        ByPlays,
        ByInstalls,
        ByName,
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

}
