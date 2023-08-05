#pragma once
#include <QByteArray>

class GZip {
   public:
    static bool unzip(const QByteArray& compressedBytes, QByteArray& uncompressedBytes);
    static bool zip(const QByteArray& uncompressedBytes, QByteArray& compressedBytes);
};
