#include "IconUtils.h"

#include "FileSystem.h"
#include <QDirIterator>

#include <array>

namespace {
std::array<const char *, 6> validIconExtensions = {{
    "svg",
    "png",
    "ico",
    "gif",
    "jpg",
    "jpeg"
}};
}

namespace IconUtils{

QString findBestIconIn(const QString &folder, const QString & iconKey) {
    int best_found = validIconExtensions.size();
    QString best_filename;

    QDirIterator it(folder, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        it.next();
        auto fileInfo = it.fileInfo();

        if(fileInfo.completeBaseName() != iconKey)
            continue;

        auto extension = fileInfo.suffix();

        for(int i = 0; i < best_found; i++) {
            if(extension == validIconExtensions[i]) {
                best_found = i;
                qCDebug(LAUNCHER_LOG) << i << " : " << fileInfo.fileName();
                best_filename = fileInfo.fileName();
            }
        }
    }
    return FS::PathCombine(folder, best_filename);
}

QString getIconFilter() {
    QString out;
    QTextStream stream(&out);
    stream << '(';
    for(size_t i = 0; i < validIconExtensions.size() - 1; i++) {
        if(i > 0) {
            stream << " ";
        }
        stream << "*." << validIconExtensions[i];
    }
    stream << " *." << validIconExtensions[validIconExtensions.size() - 1];
    stream << ')';
    return out;
}

}

