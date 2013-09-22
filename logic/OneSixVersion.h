#pragma once
#include <QtCore>
class OneSixLibrary;

class OneSixVersion : public QAbstractListModel
{
	// Things required to implement the Qt list model
public:
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation,
								int role = Qt::DisplayRole) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	// serialization/deserialization
public:
	bool toOriginalFile();
	static QSharedPointer<OneSixVersion> fromJson(QJsonObject root);
	static QSharedPointer<OneSixVersion> fromFile(QString filepath);

public:
	QList<QSharedPointer<OneSixLibrary>> getActiveNormalLibs();
	QList<QSharedPointer<OneSixLibrary>> getActiveNativeLibs();
	// called when something starts/stops messing with the object
	// FIXME: these are ugly in every possible way.
	void externalUpdateStart();
	void externalUpdateFinish();

	// data members
public:
	/// file this was read from. blank, if none
	QString original_file;
	/// the ID - determines which jar to use! ACTUALLY IMPORTANT!
	QString id;
	/// Last updated time - as a string
	QString time;
	/// Release time - as a string
	QString releaseTime;
	/// Release type - "release" or "snapshot"
	QString type;
	/**
	 * DEPRECATED: Old versions of the new vanilla launcher used this
	 * ex: "username_session_version"
	 */
	QString processArguments;
	/**
	 * arguments that should be used for launching minecraft
	 *
	 * ex: "--username ${auth_player_name} --session ${auth_session}
	 *      --version ${version_name} --gameDir ${game_directory} --assetsDir ${game_assets}"
	 */
	QString minecraftArguments;
	/**
	 * the minimum launcher version required by this version ... current is 4 (at point of
	 * writing)
	 */
	int minimumLauncherVersion = 0xDEADBEEF;
	/**
	 * The main class to load first
	 */
	QString mainClass;

	/// the list of libs - both active and inactive, native and java
	QList<QSharedPointer<OneSixLibrary>> libraries;

	/*
	FIXME: add support for those rules here? Looks like a pile of quick hacks to me though.

	"rules": [
		{
		"action": "allow"
		},
		{
		"action": "disallow",
		"os": {
			"name": "osx",
			"version": "^10\\.5\\.\\d$"
		}
		}
	],
	"incompatibilityReason": "There is a bug in LWJGL which makes it incompatible with OSX
	10.5.8. Please go to New Profile and use 1.5.2 for now. Sorry!"
	}
	*/
	// QList<Rule> rules;


};
