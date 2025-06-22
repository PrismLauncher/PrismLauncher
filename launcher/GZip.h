#pragma once
#include <QByteArray>
#include <QFile>

namespace GZip {

bool unzip(const QByteArray& compressedBytes, QByteArray& uncompressedBytes);
bool zip(const QByteArray& uncompressedBytes, QByteArray& compressedBytes);
QString readGzFileByBlocks(QFile* source, std::function<bool(const QByteArray&)> handleBlock);

}  // namespace GZip
