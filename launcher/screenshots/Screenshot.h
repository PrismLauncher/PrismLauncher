#pragma once

#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <memory>

struct ScreenShot {
    using Ptr = std::shared_ptr<ScreenShot>;

    ScreenShot(QFileInfo file) {
        m_file = file;
    }
    QFileInfo m_file;
    QString m_url;
    QString m_imgurId;
    QString m_imgurDeleteHash;
};
