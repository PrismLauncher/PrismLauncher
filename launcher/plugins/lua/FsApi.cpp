// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here only hold plain pointers across
// Lua calls that may raise errors: the C++ work happens in helpers that store their results in the static buffers
// below, which are reused (plugins only run on the main thread).

#include "FsApi.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <lua.hpp>

#include "FileSystem.h"
#include "plugins/lua/PrismApi.h"

namespace {

constexpr const char* s_handleMetatable = "prism.Folder";

struct Entry {
    QByteArray name;
    bool dir;
    qint64 size;
};

QByteArray s_result;
QByteArray s_error;
QList<Entry> s_entries;

bool fail(const QString& error)
{
    s_error = error.toUtf8();
    return false;
}

// Raises the error stored by the last failing helper
int raiseStored(lua_State* L)
{
    lua_pushlstring(L, s_error.constData(), s_error.size());
    return lua_error(L);
}

// The absolute path of rel inside root. Fails if rel is absolute or leaves root, also through symlinks. With
// linkItself, a symlink at the end of the path is checked by its own location instead of its target (to remove it).
bool resolve(const char* root, const char* rel, QString& resolved, bool linkItself = false)
{
    const QString rootPath = QString::fromUtf8(root);
    const QString relPath = QString::fromUtf8(rel);
    if (QDir::isAbsolutePath(relPath) || relPath.contains('\\') || relPath.contains(':')) {
        return fail(QString("'%1' must be a relative path using '/'").arg(relPath));
    }
    QString cleaned = QDir::cleanPath(relPath);
    if (cleaned == ".") {
        cleaned.clear();
    }
    if (cleaned == ".." || cleaned.startsWith("../")) {
        return fail(QString("'%1' is outside of the folder").arg(relPath));
    }
    resolved = cleaned.isEmpty() ? rootPath : rootPath + '/' + cleaned;

    // The closest existing ancestor must really be inside the root, after resolving symlinks
    const QString canonicalRoot = QFileInfo(rootPath).canonicalFilePath();
    if (canonicalRoot.isEmpty()) {
        return true;  // the root does not exist yet, so neither does anything below it
    }
    QString probe = linkItself && QFileInfo(resolved).isSymLink() ? QFileInfo(resolved).path() : resolved;
    while (!QFileInfo::exists(probe) && probe.size() > rootPath.size()) {
        probe = QFileInfo(probe).path();
    }
    const QString canonical = QFileInfo(probe).canonicalFilePath();
    if (canonical != canonicalRoot && !canonical.startsWith(canonicalRoot + '/')) {
        return fail(QString("'%1' is outside of the folder").arg(relPath));
    }
    return true;
}

bool readFile(const char* root, const char* rel)
{
    QString path;
    if (!resolve(root, rel, path)) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(QString("could not read '%1': %2").arg(QString::fromUtf8(rel), file.errorString()));
    }
    if (file.size() > PrismApi::s_maxFileReadSize) {
        return fail(QString("'%1' is larger than %2 bytes").arg(QString::fromUtf8(rel)).arg(PrismApi::s_maxFileReadSize));
    }
    s_result = file.read(PrismApi::s_maxFileReadSize + 1);
    if (s_result.size() > PrismApi::s_maxFileReadSize) {
        return fail(QString("'%1' is larger than %2 bytes").arg(QString::fromUtf8(rel)).arg(PrismApi::s_maxFileReadSize));
    }
    return true;
}

bool writeFile(const char* root, const char* rel, const char* data, size_t length, bool append)
{
    QString path;
    if (!resolve(root, rel, path)) {
        return false;
    }
    if (path == QString::fromUtf8(root)) {
        return fail("cannot write to the folder itself");
    }
    if (!FS::ensureFilePathExists(path)) {
        return fail(QString("could not create the folder of '%1'").arg(QString::fromUtf8(rel)));
    }
    if (append) {
        QFile file(path);
        if (!file.open(QIODevice::Append) || file.write(data, static_cast<qint64>(length)) != static_cast<qint64>(length)) {
            return fail(QString("could not write '%1': %2").arg(QString::fromUtf8(rel), file.errorString()));
        }
        return true;
    }
    // Replaced atomically, so a failure never leaves half a file
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data, static_cast<qint64>(length)) != static_cast<qint64>(length) ||
        !file.commit()) {
        return fail(QString("could not write '%1': %2").arg(QString::fromUtf8(rel), file.errorString()));
    }
    return true;
}

bool listFolder(const char* root, const char* rel)
{
    QString path;
    if (!resolve(root, rel, path)) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.isDir()) {
        return fail(QString("'%1' is not a folder").arg(QString::fromUtf8(rel)));
    }
    s_entries.clear();
    const auto entries = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name | QDir::DirsFirst);
    for (const QFileInfo& entry : entries) {
        s_entries.append({ entry.fileName().toUtf8(), entry.isDir(), entry.isDir() ? 0 : entry.size() });
    }
    return true;
}

// Sets exists and dir; fails only for invalid paths
bool stat(const char* root, const char* rel, bool& exists, bool& dir)
{
    QString path;
    if (!resolve(root, rel, path)) {
        return false;
    }
    const QFileInfo info(path);
    exists = info.exists();
    dir = info.isDir();
    return true;
}

bool makeFolder(const char* root, const char* rel)
{
    QString path;
    if (!resolve(root, rel, path)) {
        return false;
    }
    if (!QDir().mkpath(path)) {
        return fail(QString("could not create '%1'").arg(QString::fromUtf8(rel)));
    }
    return true;
}

bool removePath(const char* root, const char* rel, bool recursive)
{
    QString path;
    if (!resolve(root, rel, path, true)) {
        return false;
    }
    if (path == QString::fromUtf8(root)) {
        return fail("cannot remove the folder itself");
    }
    const QFileInfo info(path);
    if (!info.exists() && !info.isSymLink()) {
        return fail(QString("'%1' does not exist").arg(QString::fromUtf8(rel)));
    }
    bool removed;
    if (info.isDir() && !info.isSymLink()) {
        removed = recursive ? FS::deletePath(path) : QDir().rmdir(path);
        if (!removed && !recursive) {
            return fail(QString("could not remove '%1': not empty? pass { recursive = true }").arg(QString::fromUtf8(rel)));
        }
    } else {
        removed = QFile::remove(path);
    }
    if (!removed) {
        return fail(QString("could not remove '%1'").arg(QString::fromUtf8(rel)));
    }
    return true;
}

bool copyOrMove(const char* fromRoot, const char* fromRel, const char* toRoot, const char* toRel, bool move)
{
    QString from;
    QString to;
    if (!resolve(fromRoot, fromRel, from) || !resolve(toRoot, toRel, to)) {
        return false;
    }
    const QFileInfo info(from);
    if (!info.exists()) {
        return fail(QString("'%1' does not exist").arg(QString::fromUtf8(fromRel)));
    }
    if (from == QString::fromUtf8(fromRoot) && move) {
        return fail("cannot move the folder itself");
    }
    if (QFileInfo::exists(to)) {
        return fail(QString("'%1' already exists").arg(QString::fromUtf8(toRel)));
    }
    if (to.startsWith(from + '/')) {
        return fail(QString("cannot %1 '%2' into itself").arg(move ? "move" : "copy", QString::fromUtf8(fromRel)));
    }
    if (!FS::ensureFilePathExists(to)) {
        return fail(QString("could not create the folder of '%1'").arg(QString::fromUtf8(toRel)));
    }
    bool ok;
    if (move) {
        ok = FS::move(from, to);
    } else if (info.isDir()) {
        // Symlinks are not followed, so nothing outside the source folder is copied
        ok = FS::copy(from, to).followSymlinks(false).copyDirectories(true)();
    } else {
        ok = QFile::copy(from, to);
    }
    if (!ok) {
        return fail(QString("could not %1 '%2' to '%3'").arg(move ? "move" : "copy", QString::fromUtf8(fromRel), QString::fromUtf8(toRel)));
    }
    return true;
}

bool instanceGameRoot(PrismApi::InstanceSource* source, const char* id)
{
    s_result = source ? source->gameRoot(QByteArray(id)).toUtf8() : QByteArray();
    if (s_result.isEmpty()) {
        return fail(QString("the instance '%1' does not exist").arg(QString::fromUtf8(id)));
    }
    return true;
}

// Pushes a folder handle for the root at the top of the stack, which it replaces
void pushHandle(lua_State* L)
{
    lua_newuserdatauv(L, 0, 1);
    lua_insert(L, -2);
    lua_setiuservalue(L, -2, 1);
    luaL_setmetatable(L, s_handleMetatable);
}

// The root of the handle at index, which must stay on the stack while the result is used
const char* checkRoot(lua_State* L, int index)
{
    luaL_checkudata(L, index, s_handleMetatable);
    lua_getiuservalue(L, index, 1);
    const char* root = lua_tostring(L, -1);
    lua_pop(L, 1);  // still referenced by the handle, so the string stays valid
    return root;
}

int handleRead(lua_State* L)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_checkstring(L, 2);
    if (!readFile(root, rel)) {
        return raiseStored(L);
    }
    lua_pushlstring(L, s_result.constData(), s_result.size());
    s_result.clear();
    return 1;
}

int writeOrAppend(lua_State* L, bool append)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_checkstring(L, 2);
    size_t length = 0;
    const char* data = luaL_checklstring(L, 3, &length);
    if (!writeFile(root, rel, data, length, append)) {
        return raiseStored(L);
    }
    return 0;
}

int handleWrite(lua_State* L)
{
    return writeOrAppend(L, false);
}

int handleAppend(lua_State* L)
{
    return writeOrAppend(L, true);
}

int handleList(lua_State* L)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_optstring(L, 2, "");
    if (!listFolder(root, rel)) {
        return raiseStored(L);
    }
    lua_createtable(L, static_cast<int>(s_entries.size()), 0);
    for (qsizetype i = 0; i < s_entries.size(); i++) {
        const Entry& entry = s_entries.at(i);
        lua_createtable(L, 0, 3);
        lua_pushlstring(L, entry.name.constData(), entry.name.size());
        lua_setfield(L, -2, "name");
        lua_pushboolean(L, entry.dir);
        lua_setfield(L, -2, "dir");
        lua_pushinteger(L, entry.size);
        lua_setfield(L, -2, "size");
        lua_rawseti(L, -2, i + 1);
    }
    s_entries.clear();
    return 1;
}

int statResult(lua_State* L, bool wantDir)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_optstring(L, 2, "");
    bool exists = false;
    bool dir = false;
    if (!stat(root, rel, exists, dir)) {
        return raiseStored(L);
    }
    lua_pushboolean(L, wantDir ? dir : exists);
    return 1;
}

int handleExists(lua_State* L)
{
    return statResult(L, false);
}

int handleIsDir(lua_State* L)
{
    return statResult(L, true);
}

int handleMkdir(lua_State* L)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_checkstring(L, 2);
    if (!makeFolder(root, rel)) {
        return raiseStored(L);
    }
    return 0;
}

int handleRemove(lua_State* L)
{
    const char* root = checkRoot(L, 1);
    const char* rel = luaL_checkstring(L, 2);
    bool recursive = false;
    if (lua_istable(L, 3)) {
        lua_getfield(L, 3, "recursive");
        recursive = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    if (!removePath(root, rel, recursive)) {
        return raiseStored(L);
    }
    return 0;
}

int copyOrMoveFn(lua_State* L, bool move)
{
    const char* fromRoot = checkRoot(L, 1);
    const char* fromRel = luaL_checkstring(L, 2);
    const char* toRoot = checkRoot(L, 3);
    const char* toRel = luaL_checkstring(L, 4);
    if (!copyOrMove(fromRoot, fromRel, toRoot, toRel, move)) {
        return raiseStored(L);
    }
    return 0;
}

// prism.fs.copy(fromFolder, fromPath, toFolder, toPath)
int fsCopy(lua_State* L)
{
    return copyOrMoveFn(L, false);
}

// prism.fs.move(fromFolder, fromPath, toFolder, toPath)
int fsMove(lua_State* L)
{
    return copyOrMoveFn(L, true);
}

// prism.fs.data(): upvalue 1 is the data folder
int fsData(lua_State* L)
{
    lua_pushvalue(L, lua_upvalueindex(1));
    pushHandle(L);
    return 1;
}

// prism.fs.instance(id): upvalue 1 tells whether the plugin has the fs.instance permission, upvalue 2 is a light
// userdata pointing to the InstanceSource (or null)
int fsInstance(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.fs.instance requires the 'fs.instance' permission");
    }
    const char* id = luaL_checkstring(L, 1);
    if (!instanceGameRoot(static_cast<PrismApi::InstanceSource*>(lua_touserdata(L, lua_upvalueindex(2))), id)) {
        return raiseStored(L);
    }
    lua_pushlstring(L, s_result.constData(), s_result.size());
    s_result.clear();
    pushHandle(L);
    return 1;
}

}  // namespace

namespace PrismApi {

void pushFsTable(lua_State* L, const QByteArray& dataDir, bool instanceAccess, InstanceSource* instances)
{
    if (luaL_newmetatable(L, s_handleMetatable)) {
        const luaL_Reg methods[] = {
            { "read", handleRead },   { "write", handleWrite },   { "append", handleAppend },
            { "list", handleList },   { "exists", handleExists }, { "isDir", handleIsDir },
            { "mkdir", handleMkdir }, { "remove", handleRemove }, { nullptr, nullptr },
        };
        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
        lua_pushboolean(L, false);
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    lua_createtable(L, 0, 4);
    lua_pushlstring(L, dataDir.constData(), dataDir.size());
    lua_pushcclosure(L, fsData, 1);
    lua_setfield(L, -2, "data");
    lua_pushboolean(L, instanceAccess);
    lua_pushlightuserdata(L, instances);
    lua_pushcclosure(L, fsInstance, 2);
    lua_setfield(L, -2, "instance");
    lua_pushcfunction(L, fsCopy);
    lua_setfield(L, -2, "copy");
    lua_pushcfunction(L, fsMove);
    lua_setfield(L, -2, "move");
}

}  // namespace PrismApi
