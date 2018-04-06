#include "FtbListModel.h"
#include "MultiMC.h"

#include <MMCStrings.h>
#include <Version.h>

#include <QtMath>
#include <QLabel>

#include <RWStorage.h>
#include <Env.h>

FtbFilterModel::FtbFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	currentSorting = Sorting::ByGameVersion;
	sortings.insert(tr("Sort by name"), Sorting::ByName);
	sortings.insert(tr("Sort by game version"), Sorting::ByGameVersion);
}

bool FtbFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	FtbModpack leftPack = sourceModel()->data(left, Qt::UserRole).value<FtbModpack>();
	FtbModpack rightPack = sourceModel()->data(right, Qt::UserRole).value<FtbModpack>();

	if(currentSorting == Sorting::ByGameVersion) {
		Version lv(leftPack.mcVersion);
		Version rv(rightPack.mcVersion);
		return lv < rv;

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

QString FtbFilterModel::translateCurrentSorting()
{
	return sortings.key(currentSorting);
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

FtbListModel::~FtbListModel()
{
}

QString FtbListModel::translatePackType(FtbPackType type) const
{
	if(type == FtbPackType::Public) {
		return tr("Public Modpack");
	} else if(type == FtbPackType::ThirdParty) {
		return tr("Third Party Modpack");
	} else if(type == FtbPackType::Private) {
		return tr("Private Modpack");
	} else {
		return tr("Unknown Type");
	}
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
	if(pos >= modpacks.size() || pos < 0 || !index.isValid()) {
		return QString("INVALID INDEX %1").arg(pos);
	}

	FtbModpack pack = modpacks.at(pos);
	if(role == Qt::DisplayRole) {
		return pack.name + "\n" + translatePackType(pack.type);
	} else if (role == Qt::ToolTipRole) {
		if(pack.description.length() > 100) {
			//some magic to prevent to long tooltips and replace html linebreaks
			QString edit = pack.description.left(97);
			edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
			return edit;

		}
		return pack.description;
	} else if(role == Qt::DecorationRole) {
		if(m_logoMap.contains(pack.logo)) {
			return (m_logoMap.value(pack.logo));
		}
		QIcon icon = MMC->getThemedIcon("screenshot-placeholder");
		((FtbListModel *)this)->requestLogo(pack.logo);
		return icon;
	} else if(role == Qt::TextColorRole) {
		if(pack.broken) {
			//FIXME: Hardcoded color
			return QColor(255, 0, 50);
		} else if(pack.bugged) {
			//FIXME: Hardcoded color
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
	beginResetModel();
	this->modpacks = modpacks;
	endResetModel();
}

FtbModpack FtbListModel::at(int row)
{
	return modpacks.at(row);
}

void FtbListModel::logoLoaded(QString logo, QIcon out)
{
	m_loadingLogos.removeAll(logo);
	m_logoMap.insert(logo, out);
	emit dataChanged(createIndex(0, 0), createIndex(1, 0));
}

void FtbListModel::logoFailed(QString logo)
{
	m_failedLogos.append(logo);
	m_loadingLogos.removeAll(logo);
}

void FtbListModel::requestLogo(QString file)
{
	if(m_loadingLogos.contains(file) || m_failedLogos.contains(file)) {
		return;
	}

	MetaEntryPtr entry = ENV.metacache()->resolveEntry("FTBPacks", QString("logos/%1").arg(file.section(".", 0, 0)));
	NetJob *job = new NetJob(QString("FTB Icon Download for %1").arg(file));
	job->addNetAction(Net::Download::makeCached(QUrl(QString("https://ftb.cursecdn.com/FTB2/static/%1").arg(file)), entry));

	auto fullPath = entry->getFullPath();
	QObject::connect(job, &NetJob::finished, this, [this, file, fullPath]{
		emit logoLoaded(file, QIcon(fullPath));
	});

	QObject::connect(job, &NetJob::failed, this, [this, file]{
		emit logoFailed(file);
	});

	job->start();

	m_loadingLogos.append(file);
}
