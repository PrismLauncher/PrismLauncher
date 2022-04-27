#pragma once

#include "Sink.h"

namespace Net {

/*
 * Sink object for downloads that uses an external QByteArray it doesn't own as a target.
 * FIXME: It is possible that the QByteArray is freed while we're doing some operation on it,
 * causing a segmentation fault.
 */
class ByteArraySink : public Sink {
   public:
    ByteArraySink(QByteArray* output) : m_output(output){};

    virtual ~ByteArraySink() = default;

   public:
    auto init(QNetworkRequest& request) -> Task::State override
    {
        m_output->clear();
        if (initAllValidators(request))
            return Task::State::Running;
        return Task::State::Failed;
    };

    auto write(QByteArray& data) -> Task::State override
    {
        m_output->append(data);
        if (writeAllValidators(data))
            return Task::State::Running;
        return Task::State::Failed;
    }

    auto abort() -> Task::State override
    {
        m_output->clear();
        failAllValidators();
        return Task::State::Failed;
    }

    auto finalize(QNetworkReply& reply) -> Task::State override
    {
        if (finalizeAllValidators(reply))
            return Task::State::Succeeded;
        return Task::State::Failed;
    }

    auto hasLocalData() -> bool override { return false; }

   private:
    QByteArray* m_output;
};
}  // namespace Net
