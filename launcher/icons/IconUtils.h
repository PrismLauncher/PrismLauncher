#pragma once

#include <QString>

namespace IconUtils {

// Given a folder and an icon key, find 'best' of the icons with the given key in there and return its path
QString findBestIconIn(const QString &folder, const QString & iconKey);

// Get icon file type filter for file browser dialogs
QString getIconFilter();

}
