#include "MetaCacheSink.h"
#include <QFile>
#include <QFileInfo>
#include "Env.h"
#include "FileSystem.h"

namespace Net {

MetaCacheSink::MetaCacheSink(MetaEntryPtr entry, ChecksumValidator * md5sum)
    :Net::FileSink(entry->getFullPath()), m_entry(entry), m_md5Node(md5sum)
{
    addValidator(md5sum);
}

MetaCacheSink::~MetaCacheSink()
{
    // nil
}

JobStatus MetaCacheSink::initCache(QNetworkRequest& request)
{
    if (!m_entry->isStale())
    {
        return Job_Finished;
    }
    // check if file exists, if it does, use its information for the request
    QFile current(m_filename);
    if(current.exists() && current.size() != 0)
    {
        if (m_entry->getRemoteChangedTimestamp().size())
        {
            request.setRawHeader(QString("If-Modified-Since").toLatin1(), m_entry->getRemoteChangedTimestamp().toLatin1());
        }
        if (m_entry->getETag().size())
        {
            request.setRawHeader(QString("If-None-Match").toLatin1(), m_entry->getETag().toLatin1());
        }
    }
    return Job_InProgress;
}

JobStatus MetaCacheSink::finalizeCache(QNetworkReply & reply)
{
    QFileInfo output_file_info(m_filename);
    if(wroteAnyData)
    {
        m_entry->setMD5Sum(m_md5Node->hash().toHex().constData());
    }
    m_entry->setETag(reply.rawHeader("ETag").constData());
    if (reply.hasRawHeader("Last-Modified"))
    {
        m_entry->setRemoteChangedTimestamp(reply.rawHeader("Last-Modified").constData());
    }
    m_entry->setLocalChangedTimestamp(output_file_info.lastModified().toUTC().toMSecsSinceEpoch());
    m_entry->setStale(false);
    ENV->metacache()->updateEntry(m_entry);
    return Job_Finished;
}

bool MetaCacheSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}
}
