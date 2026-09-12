#include "OverrideUtils.h"

#include <QDirListing>

#include "FileSystem.h"

namespace Override {

void createOverrides(const QString& name, const QString& parentFolder, const QString& overridePath)
{
    QString filePath(FS::PathCombine(parentFolder, name + ".txt"));
    if (QFile::exists(filePath)) {
        FS::deletePath(filePath);
    }

    FS::ensureFilePathExists(filePath);

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly)) {
        qWarning() << "Failed to open file" << file.fileName() << "for writing:" << file.errorString();
        return;
    }

    for (const auto& entry : QDirListing(overridePath, QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks |
                                                           QDirListing::IteratorFlag::Recursive)) {
        // Absolute path with temp directory -> relative path
        auto overrideFilePath = entry.absoluteFilePath().split(name).last().remove(0, 1);

        file.write(overrideFilePath.toUtf8());
        file.write("\n");
    }

    file.close();
}

QStringList readOverrides(const QString& name, const QString& parentFolder)
{
    QString filePath(FS::PathCombine(parentFolder, name + ".txt"));

    QFile file(filePath);
    if (!file.exists()) {
        return {};
    }

    QStringList previousOverrides;

    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file" << file.fileName() << "for reading:" << file.errorString();
        return previousOverrides;
    }

    QString entry;
    do {
        entry = file.readLine();
        previousOverrides.append(entry.trimmed());
    } while (!entry.isEmpty());

    file.close();

    return previousOverrides;
}

}  // namespace Override
