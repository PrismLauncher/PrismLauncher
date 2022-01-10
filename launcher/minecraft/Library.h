#pragma once
#include <QString>
#include <net/NetAction.h>
#include <QPair>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QUrl>
#include <memory>

#include "Rule.h"
#include "minecraft/OpSys.h"
#include "GradleSpecifier.h"
#include "MojangDownloadInfo.h"

class Library;
class MinecraftInstance;

typedef std::shared_ptr<Library> LibraryPtr;

class Library
{
    friend class OneSixVersionFormat;
    friend class MojangVersionFormat;
    friend class LibraryTest;
public:
    Library()
    {
    }
    Library(const QString &name)
    {
        m_name = name;
    }
    /// limited copy without some data. TODO: why?
    static LibraryPtr limitedCopy(LibraryPtr base)
    {
        auto newlib = std::make_shared<Library>();
        newlib->m_name = base->m_name;
        newlib->m_repositoryURL = base->m_repositoryURL;
        newlib->m_hint = base->m_hint;
        newlib->m_absoluteURL = base->m_absoluteURL;
        newlib->m_extractExcludes = base->m_extractExcludes;
        newlib->m_nativeClassifiers = base->m_nativeClassifiers;
        newlib->m_rules = base->m_rules;
        newlib->m_storagePrefix = base->m_storagePrefix;
        newlib->m_mojangDownloads = base->m_mojangDownloads;
        newlib->m_filename = base->m_filename;
        return newlib;
    }

public: /* methods */
    /// Returns the raw name field
    const GradleSpecifier & rawName() const
    {
        return m_name;
    }

    void setRawName(const GradleSpecifier & spec)
    {
        m_name = spec;
    }

    void setClassifier(const QString & spec)
    {
        m_name.setClassifier(spec);
    }

    /// returns the full group and artifact prefix
    QString artifactPrefix() const
    {
        return m_name.artifactPrefix();
    }

    /// get the artifact ID
    QString artifactId() const
    {
        return m_name.artifactId();
    }

    /// get the artifact version
    QString version() const
    {
        return m_name.version();
    }

    /// Returns true if the library is native
    bool isNative() const
    {
        return m_nativeClassifiers.size() != 0;
    }

    void setStoragePrefix(QString prefix = QString());

    /// Set the url base for downloads
    void setRepositoryURL(const QString &base_url)
    {
        m_repositoryURL = base_url;
    }

    void getApplicableFiles(OpSys system, QStringList & jar, QStringList & native,
                            QStringList & native32, QStringList & native64, const QString & overridePath) const;

    void setAbsoluteUrl(const QString &absolute_url)
    {
        m_absoluteURL = absolute_url;
    }

    void setFilename(const QString &filename)
    {
        m_filename = filename;
    }

    /// Get the file name of the library
    QString filename(OpSys system) const;

    // DEPRECATED: set a display name, used by jar mods only
    void setDisplayName(const QString & displayName)
    {
        m_displayname = displayName;
    }

    /// Get the file name of the library
    QString displayName(OpSys system) const;

    void setMojangDownloadInfo(MojangLibraryDownloadInfo::Ptr info)
    {
        m_mojangDownloads = info;
    }

    void setHint(const QString &hint)
    {
        m_hint = hint;
    }

    /// Set the load rules
    void setRules(QList<std::shared_ptr<Rule>> rules)
    {
        m_rules = rules;
    }

    /// Returns true if the library should be loaded (or extracted, in case of natives)
    bool isActive() const;

    /// Returns true if the library is contained in an instance and false if it is shared
    bool isLocal() const;

    /// Returns true if the library is to always be checked for updates
    bool isAlwaysStale() const;

    /// Return true if the library requires forge XZ hacks
    bool isForge() const;

    // Get a list of downloads for this library
    QList<NetAction::Ptr> getDownloads(OpSys system, class HttpMetaCache * cache,
                                     QStringList & failedLocalFiles, const QString & overridePath) const;

private: /* methods */
    /// the default storage prefix used by PolyMC
    static QString defaultStoragePrefix();

    /// Get the prefix - root of the storage to be used
    QString storagePrefix() const;

    /// Get the relative file path where the library should be saved
    QString storageSuffix(OpSys system) const;

    QString hint() const
    {
        return m_hint;
    }

protected: /* data */
    /// the basic gradle dependency specifier.
    GradleSpecifier m_name;

    /// DEPRECATED URL prefix of the maven repo where the file can be downloaded
    QString m_repositoryURL;

    /// DEPRECATED: PolyMC-specific absolute URL. takes precedence over the implicit maven repo URL, if defined
    QString m_absoluteURL;

    /// PolyMC extension - filename override
    QString m_filename;

    /// DEPRECATED PolyMC extension - display name
    QString m_displayname;

    /**
     * PolyMC-specific type hint - modifies how the library is treated
     */
    QString m_hint;

    /**
     * storage - by default the local libraries folder in polymc, but could be elsewhere
     * PolyMC specific, because of FTB.
     */
    QString m_storagePrefix;

    /// true if the library had an extract/excludes section (even empty)
    bool m_hasExcludes = false;

    /// a list of files that shouldn't be extracted from the library
    QStringList m_extractExcludes;

    /// native suffixes per OS
    QMap<OpSys, QString> m_nativeClassifiers;

    /// true if the library had a rules section (even empty)
    bool applyRules = false;

    /// rules associated with the library
    QList<std::shared_ptr<Rule>> m_rules;

    /// MOJANG: container with Mojang style download info
    MojangLibraryDownloadInfo::Ptr m_mojangDownloads;
};

