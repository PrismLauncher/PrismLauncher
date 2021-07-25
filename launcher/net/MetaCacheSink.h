#pragma once
#include "FileSink.h"
#include "ChecksumValidator.h"
#include "net/HttpMetaCache.h"

namespace Net {
class MetaCacheSink : public FileSink
{
public: /* con/des */
    MetaCacheSink(MetaEntryPtr entry, ChecksumValidator * md5sum);
    virtual ~MetaCacheSink();
    bool hasLocalData() override;

protected: /* methods */
    JobStatus initCache(QNetworkRequest & request) override;
    JobStatus finalizeCache(QNetworkReply & reply) override;

private: /* data */
    MetaEntryPtr m_entry;
    ChecksumValidator * m_md5Node;
};
}
