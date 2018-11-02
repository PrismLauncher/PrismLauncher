#include "URLConstants.h"

namespace URLConstants {

QString getLegacyJarUrl(QString version)
{
    return "https://" + AWS_DOWNLOAD_VERSIONS + getJarPath(version);
}

QString getJarPath(QString version)
{
    return version + "/" + version + ".jar";
}


}
