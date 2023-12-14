#pragma once

#include <QString>
#include <QUrl>

class QFileInfo;

/**
 * This wraps around QDesktopServices and adds workarounds where needed
 * Use this instead of QDesktopServices!
 */
namespace DesktopServices {
/**
 * Open a path in whatever application is applicable.
 * @param ensurePathExists Make sure the path exists
 */
bool openPath(const QFileInfo& path, bool ensurePathExists = false);

/**
 * Open a path in whatever application is applicable.
 * @param ensurePathExists Make sure the path exists
 */
bool openPath(const QString& path, bool ensurePathExists = false);

/**
 * Run an application
 */
bool run(const QString& application, const QStringList& args, const QString& workingDirectory = QString(), qint64* pid = 0);

/**
 * Open the URL, most likely in a browser. Maybe.
 */
bool openUrl(const QUrl& url);

/**
 * Determine whether the launcher is running in a Flatpak environment
 */
bool isFlatpak();

/**
 * Determine whether the launcher is running in a Snap environment
 */
bool isSnap();

/**
 * Determine whether the launcher is running in a sandboxed (Flatpak or Snap) environment
 */
bool isSandbox();
}  // namespace DesktopServices
