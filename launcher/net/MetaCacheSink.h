#pragma once

#include "ChecksumValidator.h"
#include "FileSink.h"
#include "net/HttpMetaCache.h"

namespace Net {
class MetaCacheSink : public FileSink {
   public:
    MetaCacheSink(MetaEntryPtr entry, ChecksumValidator* md5sum);
    virtual ~MetaCacheSink() = default;

    auto hasLocalData() -> bool override;

   protected:
    auto initCache(QNetworkRequest& request) -> Task::State override;
    auto finalizeCache(QNetworkReply& reply) -> Task::State override;

   private:
    MetaEntryPtr m_entry;
    ChecksumValidator* m_md5Node;
};
}  // namespace Net
