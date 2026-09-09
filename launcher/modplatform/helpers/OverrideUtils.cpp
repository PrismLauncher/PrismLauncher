#include "OverrideUtils.h"

#include <QDirListing>

#include "FileSystem.h"

namespace Override {

void createOverrides(const QString& name, const QString& parent_folder, const QString& override_path)
{
    QString file_path(FS::PathCombine(parent_folder, name + ".txt"));
    if (QFile::exists(file_path))
        FS::deletePath(file_path);

    FS::ensureFilePathExists(file_path);

    QFile file(file_path);
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

QStringList readOverrides(const QString& name, const QString& parent_folder)
{
    QString file_path(FS::PathCombine(parent_folder, name + ".txt"));

    QFile file(file_path);
    if (!file.exists())
        return {};

    QStringList previous_overrides;

    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file" << file.fileName() << "for reading:" << file.errorString();
        return previous_overrides;
    }

    QString entry;
    do {
        entry = file.readLine();
        previous_overrides.append(entry.trimmed());
    } while (!entry.isEmpty());

    file.close();

    return previous_overrides;
}

}  // namespace Override
