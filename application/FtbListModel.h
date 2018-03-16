#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <modplatform/ftb/PackHelpers.h>

class FtbFilterModel : public QSortFilterProxyModel
{
public:
	FtbFilterModel(QObject* parent = Q_NULLPTR);
	enum Sorting {
		ByName,
		ByGameVersion
	};
	const QMap<QString, Sorting> getAvailableSortings();
	Sorting getCurrentSorting();
	void setSorting(Sorting sorting);

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

public:
	FtbListModel(QObject *parent);
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	void fill(FtbModpackList modpacks);

	FtbModpack at(int row);

};
