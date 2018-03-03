#include "FtbListModel.h"
#include "MultiMC.h"

#include <MMCStrings.h>

#include <QtMath>

FtbFilterModel::FtbFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	currentSorting = Sorting::ByGameVersion;
	sortings.insert("Sort by name", Sorting::ByName);
	sortings.insert("Sort by game version", Sorting::ByGameVersion);
}

bool FtbFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	FtbModpack leftPack = sourceModel()->data(left, Qt::UserRole).value<FtbModpack>();
	FtbModpack rightPack = sourceModel()->data(right, Qt::UserRole).value<FtbModpack>();

	if(currentSorting == Sorting::ByGameVersion) {
		QStringList leftList = leftPack.mcVersion.split(".");
		QStringList rightList = rightPack.mcVersion.split(".");

		if(leftList.size() < 1) {
			return true;
		} else if(rightList.size() < 1) {
			return false;
		} else {
			for(int i = 0; i < qMax(leftList.size(), rightList.size()); i++) {
				if(leftList.size() -1 < i) {
					return true;
				}

				if(rightList.size() -1 < i) {
					return false;
				}

				int leftV = leftList.at(i).toInt();
				int rightV = rightList.at(i).toInt();

				if(leftV != rightV) {
					return leftV < rightV;
				}

			}
			return false;
		}

	} else if(currentSorting == Sorting::ByName) {
		return Strings::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
	}

	//UHM, some inavlid value set?!
	qWarning() << "Invalid sorting set!";
	return true;
}

bool FtbFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	return true;
}

const QMap<QString, FtbFilterModel::Sorting> FtbFilterModel::getAvailableSortings()
{
	return sortings;
}

void FtbFilterModel::setSorting(Sorting s)
{
	currentSorting = s;
	invalidate();
}

FtbFilterModel::Sorting FtbFilterModel::getCurrentSorting()
{
	return currentSorting;
}

FtbListModel::FtbListModel(QObject *parent) : QAbstractListModel(parent)
{
}

int FtbListModel::rowCount(const QModelIndex &parent) const
{
	return modpacks.size();
}

int FtbListModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant FtbListModel::data(const QModelIndex &index, int role) const
{
	int pos = index.row();
	if(modpacks.size() < pos || pos < 0) {
		return QString("INVALID INDEX %1").arg(pos);
	}

	FtbModpack pack = modpacks.at(pos);

	if(role == Qt::DisplayRole) {
		return pack.name;
	} else if (role == Qt::ToolTipRole) {
		if(pack.description.length() > 100) {
			//some magic to prevent to long tooltips and replace html linebreaks
			QString edit = pack.description.left(97);
			edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
			return edit;

		}
		return pack.description;
	} else if(role == Qt::DecorationRole) {
		//TODO: Add pack logos or something... but they have a weird size. This needs some design hacks
	} else if(role == Qt::TextColorRole) {
		if(pack.broken) {
			return QColor(255, 0, 50);
		} else if(pack.bugged) {
			//bugged pack, currently only indicates bugged xml
			return QColor(244, 229, 66);
		}
	} else if(role == Qt::UserRole) {
		QVariant v;
		v.setValue(pack);
		return v;
	}
	return QVariant();
}

void FtbListModel::fill(FtbModpackList modpacks)
{
	this->modpacks = modpacks;
}

FtbModpack FtbListModel::at(int row)
{
	return modpacks.at(row);
}
