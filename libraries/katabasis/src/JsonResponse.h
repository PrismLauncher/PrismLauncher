#pragma once

#include <QVariantMap>

class QByteArray;

namespace Katabasis {

    /// Parse JSON data into a QVariantMap
QVariantMap parseJsonResponse(const QByteArray &data);

}
