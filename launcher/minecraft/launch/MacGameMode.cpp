// SPDX-License-Identifier: GPL-3.0-only

#include <QByteArrayView>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QOperatingSystemVersion>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>

namespace {
#include "MacGameModeBin.h"

std::atomic<uint64_t> s_nextTemp{ 0 };
}  // namespace

QString prepareMacGameMode(const QString& rootDir)
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 14)) {
        return {};
    }
    if (rootDir.isEmpty()) {
        return {};
    }

    QDir root(rootDir);
    auto helpersDir = root.filePath("helpers");
    auto target = QDir(helpersDir).filePath("gamemode");
    auto targetEncoded = QFile::encodeName(target);
    const QByteArrayView stubBytes(reinterpret_cast<const char*>(kMacGameMode), static_cast<qsizetype>(kMacGameMode_len));

    struct stat info{};
    if (lstat(targetEncoded.constData(), &info) == 0 && S_ISREG(info.st_mode) && (info.st_mode & 0777) == 0755 &&
        info.st_size == stubBytes.size()) {
        QFile existing(target);
        if (existing.open(QIODevice::ReadOnly) && existing.readAll() == stubBytes) {
            return target;
        }
    }

    if (!QDir().mkpath(helpersDir)) {
        return {};
    }

    QByteArray tempEncoded;
    int fd = -1;
    while (fd < 0) {
        auto tempPath = QDir(helpersDir)
                            .filePath(QString(".game-mode-%1-%2")
                                          .arg(QCoreApplication::applicationPid())
                                          .arg(s_nextTemp.fetch_add(1, std::memory_order_relaxed)));
        tempEncoded = QFile::encodeName(tempPath);
        fd = open(tempEncoded.constData(), O_WRONLY | O_CREAT | O_EXCL, 0700);
        if (fd < 0 && errno != EEXIST) {
            return {};
        }
    }

    bool ok = true;
    const char* ptr = stubBytes.data();
    qsizetype remaining = stubBytes.size();
    while (remaining > 0) {
        ssize_t written = write(fd, ptr, static_cast<size_t>(remaining));
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            ok = false;
            break;
        }
        ptr += written;
        remaining -= written;
    }

    if (ok && fchmod(fd, 0755) != 0) {
        ok = false;
    }
    if (ok && fsync(fd) != 0) {
        ok = false;
    }
    if (close(fd) != 0) {
        ok = false;
    }
    if (ok && rename(tempEncoded.constData(), targetEncoded.constData()) != 0) {
        ok = false;
    }

    if (!ok) {
        unlink(tempEncoded.constData());
        return {};
    }
    return target;
}
