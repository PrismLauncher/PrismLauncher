#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/Rule.h"
#include "ProblemProvider.h"
#include "Library.h"
#include <meta/JsonFormat.h>

class PackProfile;
class VersionFile;
class LaunchProfile;
struct MojangDownloadInfo;
struct MojangAssetIndexInfo;

using VersionFilePtr = std::shared_ptr<VersionFile>;
class VersionFile : public ProblemContainer
{
    friend class MojangVersionFormat;
    friend class OneSixVersionFormat;
public: /* methods */
    void applyTo(LaunchProfile* profile);

public: /* data */
    /// PolyMC: order hint for this version file if no explicit order is set
    int order = 0;

    /// PolyMC: human readable name of this package
    QString name;

    /// PolyMC: package ID of this package
    QString uid;

    /// PolyMC: version of this package
    QString version;

    /// PolyMC: DEPRECATED dependency on a Minecraft version
    QString dependsOnMinecraftVersion;

    /// Mojang: DEPRECATED used to version the Mojang version format
    int minimumLauncherVersion = -1;

    /// Mojang: DEPRECATED version of Minecraft this is
    QString minecraftVersion;

    /// Mojang: class to launch Minecraft with
    QString mainClass;

    /// PolyMC: class to launch legacy Minecraft with (embed in a custom window)
    QString appletClass;

    /// Mojang: Minecraft launch arguments (may contain placeholders for variable substitution)
    QString minecraftArguments;

    /// Mojang: list of compatible java majors
    QList<int> compatibleJavaMajors;

    /// Mojang: type of the Minecraft version
    QString type;

    /// Mojang: the time this version was actually released by Mojang
    QDateTime releaseTime;

    /// Mojang: DEPRECATED the time this version was last updated by Mojang
    QDateTime updateTime;

    /// Mojang: DEPRECATED asset group to be used with Minecraft
    QString assets;

    /// PolyMC: list of tweaker mod arguments for launchwrapper
    QStringList addTweakers;

    /// Mojang: list of libraries to add to the version
    QList<LibraryPtr> libraries;

    /// PolyMC: list of maven files to put in the libraries folder, but not in classpath
    QList<LibraryPtr> mavenFiles;

    /// The main jar (Minecraft version library, normally)
    LibraryPtr mainJar;

    /// PolyMC: list of attached traits of this version file - used to enable features
    QSet<QString> traits;

    /// PolyMC: list of jar mods added to this version
    QList<LibraryPtr> jarMods;

    /// PolyMC: list of mods added to this version
    QList<LibraryPtr> mods;

    /**
     * PolyMC: set of packages this depends on
     * NOTE: this is shared with the meta format!!!
     */
    Meta::RequireSet requires;

    /**
     * PolyMC: set of packages this conflicts with
     * NOTE: this is shared with the meta format!!!
     */
    Meta::RequireSet conflicts;

    /// is volatile -- may be removed as soon as it is no longer needed by something else
    bool m_volatile = false;

public:
    // Mojang: DEPRECATED list of 'downloads' - client jar, server jar, windows server exe, maybe more.
    QMap <QString, std::shared_ptr<MojangDownloadInfo>> mojangDownloads;

    // Mojang: extended asset index download information
    std::shared_ptr<MojangAssetIndexInfo> mojangAssetIndex;
};

