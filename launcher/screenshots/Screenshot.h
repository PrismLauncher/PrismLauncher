#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QString>
#include <memory>
#include <utility>

struct ScreenShot {
    using Ptr = std::shared_ptr<ScreenShot>;

    ScreenShot(QFileInfo file) { m_file = std::move(file); }
    QFileInfo m_file;
    QString m_url;
    QString m_imgurId;
    QString m_imgurDeleteHash;
};
