#pragma once

#include <QSaveFile>

#include "Sink.h"

namespace Net {
class FileSink : public Sink {
   public:
    FileSink(QString filename) : m_filename(filename){};
    virtual ~FileSink() = default;

   public:
    auto init(QNetworkRequest& request) -> Task::State override;
    auto write(QByteArray& data) -> Task::State override;
    auto abort() -> Task::State override;
    auto finalize(QNetworkReply& reply) -> Task::State override;

    auto hasLocalData() -> bool override;

   protected:
    virtual auto initCache(QNetworkRequest&) -> Task::State;
    virtual auto finalizeCache(QNetworkReply& reply) -> Task::State;

   protected:
    QString m_filename;
    bool wroteAnyData = false;
    std::unique_ptr<QSaveFile> m_output_file;
};
}  // namespace Net
