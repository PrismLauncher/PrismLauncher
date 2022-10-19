#include "PackageManifest.h"
#include <Json.h>
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QDebug>
#include "launcherlog.h"

#ifndef Q_OS_WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace mojang_files {

const Hash hash_of_empty_string = "da39a3ee5e6b4b0d3255bfef95601890afd80709";

int Path::compare(const Path& rhs) const
{
    auto left_cursor = begin();
    auto left_end = end();
    auto right_cursor = rhs.begin();
    auto right_end = rhs.end();

    while (left_cursor != left_end && right_cursor != right_end)
    {
        if(*left_cursor < *right_cursor)
        {
            return -1;
        }
        else if(*left_cursor > *right_cursor)
        {
            return 1;
        }
        left_cursor++;
        right_cursor++;
    }

    if(left_cursor == left_end)
    {
        if(right_cursor == right_end)
        {
            return 0;
        }
        return -1;
    }
    return 1;
}

void Package::addFile(const Path& path, const File& file) {
    addFolder(path.parent_path());
    files[path] = file;
}

void Package::addFolder(Path folder) {
    if(!folder.has_parent_path()) {
        return;
    }
    do {
        folders.insert(folder);
        folder = folder.parent_path();
    } while(folder.has_parent_path());
}

void Package::addLink(const Path& path, const Path& target) {
    addFolder(path.parent_path());
    symlinks[path] = target;
}

void Package::addSource(const FileSource& source) {
    sources[source.hash] = source;
}


namespace {
void fromJson(QJsonDocument & doc, Package & out) {
    std::set<Path> seen_paths;
    if (!doc.isObject())
    {
        throw JSONValidationError("file manifest is not an object");
    }
    QJsonObject root = doc.object();

    auto filesObj = Json::ensureObject(root, "files");
    auto iter = filesObj.begin();
    while (iter != filesObj.end())
    {
        Path objectPath = Path(iter.key());
        auto value = iter.value();
        iter++;
        if(seen_paths.count(objectPath)) {
            throw JSONValidationError("duplicate path inside manifest, the manifest is invalid");
        }
        if (!value.isObject())
        {
            throw JSONValidationError("file entry inside manifest is not an an object");
        }
        seen_paths.insert(objectPath);

        auto fileObject = value.toObject();
        auto type = Json::requireString(fileObject, "type");
        if(type == "directory") {
            out.addFolder(objectPath);
            continue;
        }
        else if(type == "file") {
            FileSource bestSource;
            File file;
            file.executable = Json::ensureBoolean(fileObject, QString("executable"), false);
            auto downloads = Json::requireObject(fileObject, "downloads");
            for(auto iter2 = downloads.begin(); iter2 != downloads.end(); iter2++) {
                FileSource source;

                auto downloadObject = Json::requireObject(iter2.value());
                source.hash = Json::requireString(downloadObject, "sha1");
                source.size = Json::requireInteger(downloadObject, "size");
                source.url = Json::requireString(downloadObject, "url");

                auto compression = iter2.key();
                if(compression == "raw") {
                    file.hash = source.hash;
                    file.size = source.size;
                    source.compression = Compression::Raw;
                }
                else if (compression == "lzma") {
                    source.compression = Compression::Lzma;
                }
                else {
                    continue;
                }
                bestSource.upgrade(source);
            }
            if(bestSource.isBad()) {
                throw JSONValidationError("No valid compression method for file " + iter.key());
            }
            out.addFile(objectPath, file);
            out.addSource(bestSource);
        }
        else if(type == "link") {
            auto target = Json::requireString(fileObject, "target");
            out.symlinks[objectPath] = target;
            out.addLink(objectPath, target);
        }
        else {
            throw JSONValidationError("Invalid item type in manifest: " + type);
        }
    }
    // make sure the containing folder exists
    out.folders.insert(Path());
}
}

Package Package::fromManifestContents(const QByteArray& contents)
{
    Package out;
    try
    {
        auto doc = Json::requireDocument(contents, "Manifest");
        fromJson(doc, out);
        return out;
    }
    catch (const Exception &e)
    {
        qCDebug(LAUNCHER_LOG) << QString("Unable to parse manifest: %1").arg(e.cause());
        out.valid = false;
        return out;
    }
}

Package Package::fromManifestFile(const QString & filename) {
    Package out;
    try
    {
        auto doc = Json::requireDocument(filename, filename);
        fromJson(doc, out);
        return out;
    }
    catch (const Exception &e)
    {
        qCDebug(LAUNCHER_LOG) << QString("Unable to parse manifest file %1: %2").arg(filename, e.cause());
        out.valid = false;
        return out;
    }
}

#ifndef Q_OS_WIN32

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace {
// FIXME: Qt obscures symlink targets by making them absolute. that is useless. this is the workaround - we do it ourselves
bool actually_read_symlink_target(const QString & filepath, Path & out)
{
    struct ::stat st;
    // FIXME: here, we assume the native filesystem encoding. May the Gods have mercy upon our Souls.
    QByteArray nativePath = filepath.toUtf8();
    const char * filepath_cstr = nativePath.data();

    if (lstat(filepath_cstr, &st) != 0)
    {
        return false;
    }

    auto size = st.st_size ? st.st_size + 1 : PATH_MAX;
    std::string temp(size, '\0');
    // because we don't realiably know how long the damn thing actually is, we loop and expand. POSIX is naff
    do
    {
        auto link_length = ::readlink(filepath_cstr, &temp[0], temp.size());
        if(link_length == -1)
        {
            return false;
        }
        if(std::string::size_type(link_length) < temp.size())
        {
            // buffer was long enough and we managed to read the link target. RETURN here.
            temp.resize(link_length);
            out = Path(QString::fromUtf8(temp.c_str()));
            return true;
        }
        temp.resize(temp.size() * 2);
    } while (true);
}
}
#endif

// FIXME: Qt filesystem abstraction is bad, but ... let's hope it doesn't break too much?
// FIXME: The error handling is just DEFICIENT
Package Package::fromInspectedFolder(const QString& folderPath)
{
    QDir root(folderPath);

    Package out;
    QDirIterator iterator(folderPath, QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System | QDir::Hidden, QDirIterator::Subdirectories);
    while(iterator.hasNext()) {
        iterator.next();

        auto fileInfo = iterator.fileInfo();
        auto relPath = root.relativeFilePath(fileInfo.filePath());
        // FIXME: this is probably completely busted on Windows anyway, so just disable it.
        // Qt makes shit up and doesn't understand the platform details
        // TODO: Actually use a filesystem library that isn't terrible and has decen license.
        //       I only know one, and I wrote it. Sadly, currently proprietary. PAIN.
#ifndef Q_OS_WIN32
        if(fileInfo.isSymLink()) {
            Path targetPath;
            if(!actually_read_symlink_target(fileInfo.filePath(), targetPath)) {
                qCCritical(LAUNCHER_LOG) << "Folder inspection: Unknown filesystem object:" << fileInfo.absoluteFilePath();
                out.valid = false;
            }
            out.addLink(relPath, targetPath);
        }
        else
#endif
        if(fileInfo.isDir()) {
            out.addFolder(relPath);
        }
        else if(fileInfo.isFile()) {
            File f;
            f.executable = fileInfo.isExecutable();
            f.size = fileInfo.size();
            // FIXME: async / optimize the hashing
            QFile input(fileInfo.absoluteFilePath());
            if(!input.open(QIODevice::ReadOnly)) {
                qCCritical(LAUNCHER_LOG) << "Folder inspection: Failed to open file:" << fileInfo.absoluteFilePath();
                out.valid = false;
                break;
            }
            f.hash = QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha1).toHex().constData();
            out.addFile(relPath, f);
        }
        else {
            // Something else... oh my
            qCCritical(LAUNCHER_LOG) << "Folder inspection: Unknown filesystem object:" << fileInfo.absoluteFilePath();
            out.valid = false;
            break;
        }
    }
    out.folders.insert(Path("."));
    out.valid = true;
    return out;
}

namespace {
struct shallow_first_sort
{
    bool operator()(const Path &lhs, const Path &rhs) const
    {
        auto lhs_depth = lhs.length();
        auto rhs_depth = rhs.length();
        if(lhs_depth < rhs_depth)
        {
            return true;
        }
        else if(lhs_depth == rhs_depth)
        {
            if(lhs < rhs)
            {
                return true;
            }
        }
        return false;
    }
};

struct deep_first_sort
{
    bool operator()(const Path &lhs, const Path &rhs) const
    {
        auto lhs_depth = lhs.length();
        auto rhs_depth = rhs.length();
        if(lhs_depth > rhs_depth)
        {
            return true;
        }
        else if(lhs_depth == rhs_depth)
        {
            if(lhs < rhs)
            {
                return true;
            }
        }
        return false;
    }
};
}

UpdateOperations UpdateOperations::resolve(const Package& from, const Package& to)
{
    UpdateOperations out;

    if(!from.valid || !to.valid) {
        out.valid = false;
        return out;
    }

    // Files
    for(auto iter = from.files.begin(); iter != from.files.end(); iter++) {
        const auto &current_hash = iter->second.hash;
        const auto &current_executable = iter->second.executable;
        const auto &path = iter->first;

        auto iter2 = to.files.find(path);
        if(iter2 == to.files.end()) {
            // removed
            out.deletes.push_back(path);
            continue;
        }
        auto new_hash = iter2->second.hash;
        auto new_executable = iter2->second.executable;
        if (current_hash != new_hash) {
            out.deletes.push_back(path);
            out.downloads.emplace(
                std::pair<Path, FileDownload>{
                    path,
                    FileDownload(to.sources.at(iter2->second.hash), iter2->second.executable)
                }
            );
        }
        else if (current_executable != new_executable) {
            out.executable_fixes[path] = new_executable;
        }
    }
    for(auto iter = to.files.begin(); iter != to.files.end(); iter++) {
        auto path = iter->first;
        if(!from.files.count(path)) {
            out.downloads.emplace(
                std::pair<Path, FileDownload>{
                    path,
                    FileDownload(to.sources.at(iter->second.hash), iter->second.executable)
                }
            );
        }
    }

    // Folders
    std::set<Path, deep_first_sort> remove_folders;
    std::set<Path, shallow_first_sort> make_folders;
    for(auto from_path: from.folders) {
        auto iter = to.folders.find(from_path);
        if(iter == to.folders.end()) {
            remove_folders.insert(from_path);
        }
    }
    for(auto & rmdir: remove_folders) {
        out.rmdirs.push_back(rmdir);
    }
    for(auto to_path: to.folders) {
        auto iter = from.folders.find(to_path);
        if(iter == from.folders.end()) {
            make_folders.insert(to_path);
        }
    }
    for(auto & mkdir: make_folders) {
        out.mkdirs.push_back(mkdir);
    }

    // Symlinks
    for(auto iter = from.symlinks.begin(); iter != from.symlinks.end(); iter++) {
        const auto &current_target = iter->second;
        const auto &path = iter->first;

        auto iter2 = to.symlinks.find(path);
        if(iter2 == to.symlinks.end()) {
            // removed
            out.deletes.push_back(path);
            continue;
        }
        const auto &new_target = iter2->second;
        if (current_target != new_target) {
            out.deletes.push_back(path);
            out.mklinks[path] = iter2->second;
        }
    }
    for(auto iter = to.symlinks.begin(); iter != to.symlinks.end(); iter++) {
        auto path = iter->first;
        if(!from.symlinks.count(path)) {
            out.mklinks[path] = iter->second;
        }
    }
    out.valid = true;
    return out;
}

}
