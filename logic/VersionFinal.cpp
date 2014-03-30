/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VersionFinal.h"

#include <QDebug>
#include <QFile>
#include <QDir>

#include "OneSixVersionBuilder.h"
#include "OneSixInstance.h"

template <typename A, typename B> QMap<A, B> invert(const QMap<B, A> &in)
{
	QMap<A, B> out;
	for (auto it = in.begin(); it != in.end(); ++it)
	{
		out.insert(it.value(), it.key());
	}
	return out;
}

VersionFinal::VersionFinal(OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
	clear();
}

void VersionFinal::reload(const bool onlyVanilla, const QStringList &external)
{
	//FIXME: source of epic failure.
	beginResetModel();
	OneSixVersionBuilder::build(this, m_instance, onlyVanilla, external);
	reapply(true);
	endResetModel();
}

void VersionFinal::clear()
{
	id.clear();
	time.clear();
	releaseTime.clear();
	type.clear();
	assets.clear();
	processArguments.clear();
	minecraftArguments.clear();
	minimumLauncherVersion = 0xDEADBEAF;
	mainClass.clear();
	libraries.clear();
	tweakers.clear();
}

bool VersionFinal::canRemove(const int index) const
{
	if (index < versionFiles.size())
	{
		return versionFiles.at(index)->fileId != "org.multimc.version.json";
	}
	return false;
}

bool VersionFinal::remove(const int index)
{
	if (canRemove(index) && QFile::remove(versionFiles.at(index)->filename))
	{
		beginResetModel();
		versionFiles.removeAt(index);
		reapply(true);
		endResetModel();
		return true;
	}
	return false;
}

bool VersionFinal::remove(const QString id)
{
	int i = 0;
	for (auto file : versionFiles)
	{
		if (file->fileId == id)
		{
			return remove(i);
		}
		i++;
	}
	return false;
}

QString VersionFinal::versionFileId(const int index) const
{
	if (index < 0 || index >= versionFiles.size())
	{
		return QString();
	}
	return versionFiles.at(index)->fileId;
}

VersionFilePtr VersionFinal::versionFile(const QString &id)
{
	for (auto file : versionFiles)
	{
		if (file->fileId == id)
		{
			return file;
		}
	}
	return 0;
}

bool VersionFinal::hasFtbPack()
{
	return versionFile("org.multimc.ftb.pack.json") != nullptr;
}

bool VersionFinal::removeFtbPack()
{
	return remove("org.multimc.ftb.pack.json");
}

QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNormalLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}
QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNativeLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

std::shared_ptr<VersionFinal> VersionFinal::fromJson(const QJsonObject &obj)
{
	std::shared_ptr<VersionFinal> version(new VersionFinal(0));
	try
	{
		OneSixVersionBuilder::readJsonAndApplyToVersion(version.get(), obj);
	}
	catch(MMCError & err)
	{
		return 0;
	}
	return version;
}

QVariant VersionFinal::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= versionFiles.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return versionFiles.at(row)->name;
		case 1:
			return versionFiles.at(row)->version;
		default:
			return QVariant();
		}
	}
	return QVariant();
}
QVariant VersionFinal::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0:
				return tr("Name");
			case 1:
				return tr("Version");
			default:
				return QVariant();
			}
		}
	}
	return QVariant();
}
Qt::ItemFlags VersionFinal::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int VersionFinal::rowCount(const QModelIndex &parent) const
{
	return versionFiles.size();
}

int VersionFinal::columnCount(const QModelIndex &parent) const
{
	return 2;
}

bool VersionFinal::isCustom()
{
	return QDir(m_instance->instanceRoot()).exists("custom.json");
}
bool VersionFinal::revertToBase()
{
	return QDir(m_instance->instanceRoot()).remove("custom.json");
}

QMap<QString, int> VersionFinal::getExistingOrder() const
{

	QMap<QString, int> order;
	// default
	{
		for (auto file : versionFiles)
		{
			order.insert(file->fileId, file->order);
		}
	}
	// overriden
	{
		QMap<QString, int> overridenOrder = OneSixVersionBuilder::readOverrideOrders(m_instance);
		for (auto id : order.keys())
		{
			if (overridenOrder.contains(id))
			{
				order[id] = overridenOrder[id];
			}
		}
	}
	return order;
}

void VersionFinal::move(const int index, const MoveDirection direction)
{
	int theirIndex;
	if (direction == MoveUp)
	{
		theirIndex = index - 1;
	}
	else
	{
		theirIndex = index + 1;
	}
	if (theirIndex < 0 || theirIndex >= versionFiles.size())
	{
		return;
	}
	const QString ourId = versionFileId(index);
	const QString theirId = versionFileId(theirIndex);
	if (ourId.isNull() || ourId.startsWith("org.multimc.") ||
			theirId.isNull() || theirId.startsWith("org.multimc."))
	{
		return;
	}

	VersionFilePtr we = versionFiles[index];
	VersionFilePtr them = versionFiles[theirIndex];
	if (!we || !them)
	{
		return;
	}
	beginMoveRows(QModelIndex(), index, index, QModelIndex(), theirIndex);
	versionFiles.replace(theirIndex, we);
	versionFiles.replace(index, them);
	endMoveRows();

	auto order = getExistingOrder();
	order[ourId] = theirIndex;
	order[theirId] = index;

	if (!OneSixVersionBuilder::writeOverrideOrders(order, m_instance))
	{
		throw MMCError(tr("Couldn't save the new order"));
	}
	else
	{
		reapply();
	}
}
void VersionFinal::resetOrder()
{
	QDir(m_instance->instanceRoot()).remove("order.json");
	reapply();
}

void VersionFinal::reapply(const bool alreadyReseting)
{
	if (!alreadyReseting)
	{
		beginResetModel();
	}

	clear();

	auto existingOrders = getExistingOrder();
	QList<int> orders = existingOrders.values();
	std::sort(orders.begin(), orders.end());
	QList<VersionFilePtr> newVersionFiles;
	for (auto order : orders)
	{
		auto file = versionFile(existingOrders.key(order));
		newVersionFiles.append(file);
		file->applyTo(this);
	}
	versionFiles.swap(newVersionFiles);
	finalize();
	if (!alreadyReseting)
	{
		endResetModel();
	}
}

void VersionFinal::finalize()
{
	if (assets.isEmpty())
	{
		assets = "legacy";
	}
	if (minecraftArguments.isEmpty())
	{
		QString toCompare = processArguments.toLower();
		if (toCompare == "legacy")
		{
			minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			minecraftArguments = "--username ${auth_player_name} "
								 "--session ${auth_session} "
								 "--version ${profile_name}";
		}
	}
}
