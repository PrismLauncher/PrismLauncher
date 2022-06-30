#pragma once

#include <QString>

namespace JavaDownloader {
    /*Downloads the java to the runtimes folder*/
    void downloadJava(bool isLegacy, const QString& OS);
}
