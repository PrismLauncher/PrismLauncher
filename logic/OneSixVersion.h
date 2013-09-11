#pragma once
#include <QtCore>
class OneSixLibrary;

class OneSixVersion
{
public:
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
	 * the minimum launcher version required by this version ... current is 4 (at point of writing)
	 */
	int minimumLauncherVersion;
	/**
	 * The main class to load first
	 */
	QString mainClass;
	
	/// the list of libs - both active and inactive, native and java
	QList<QSharedPointer<OneSixLibrary> > libraries;
	
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
	"incompatibilityReason": "There is a bug in LWJGL which makes it incompatible with OSX 10.5.8. Please go to New Profile and use 1.5.2 for now. Sorry!"
	}
	*/
	// QList<Rule> rules;
	
public:
	OneSixVersion()
	{
		minimumLauncherVersion = 0xDEADBEEF;
	}
	
	QList<QSharedPointer<OneSixLibrary> > getActiveNormalLibs();
	QList<QSharedPointer<OneSixLibrary> > getActiveNativeLibs();
};