#pragma once

#include <QUrl>
#include <QString>

/**
 * This wraps around QDesktopServices and adds workarounds where needed
 * Use this instead of QDesktopServices!
 */
namespace DesktopServices
{
    /**
     * Open a file in whatever application is applicable
     */
    bool openFile(const QString &path);

    /**
     * Open a file in the specified application
     */
    bool openFile(const QString &application, const QString &path, const QString & workingDirectory = QString(), qint64 *pid = 0);

    /**
     * Run an application
     */
    bool run(const QString &application,const QStringList &args, const QString & workingDirectory = QString(), qint64 *pid = 0);

    /**
     * Open a directory
     */
    bool openDirectory(const QString &path, bool ensureExists = false);

    /**
     * Open the URL, most likely in a browser. Maybe.
     */
    bool openUrl(const QUrl &url);

    bool isFlatpak();
}
