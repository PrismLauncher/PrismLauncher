#include "FileSink.h"

#include <QFile>

#include "FileSystem.h"

namespace Net {

Task::State FileSink::init(QNetworkRequest& request)
{
    auto result = initCache(request);
    if(result != Task::State::Running)
    {
        return result;
    }
    // create a new save file and open it for writing
    if (!FS::ensureFilePathExists(m_filename))
    {
        qCritical() << "Could not create folder for " + m_filename;
        return Task::State::Failed;
    }
    wroteAnyData = false;
    m_output_file.reset(new QSaveFile(m_filename));
    if (!m_output_file->open(QIODevice::WriteOnly))
    {
        qCritical() << "Could not open " + m_filename + " for writing";
        return Task::State::Failed;
    }

    if(initAllValidators(request))
        return Task::State::Running;
    return Task::State::Failed;
}

Task::State FileSink::initCache(QNetworkRequest &)
{
    return Task::State::Running;
}

Task::State FileSink::write(QByteArray& data)
{
    if (!writeAllValidators(data) || m_output_file->write(data) != data.size())
    {
        qCritical() << "Failed writing into " + m_filename;
        m_output_file->cancelWriting();
        m_output_file.reset();
        wroteAnyData = false;
        return Task::State::Failed;
    }
    wroteAnyData = true;
    return Task::State::Running;
}

Task::State FileSink::abort()
{
    m_output_file->cancelWriting();
    failAllValidators();
    return Task::State::Failed;
}

Task::State FileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);
    if(validStatus)
    {
        // this leaves out 304 Not Modified
        gotFile = statusCode == 200 || statusCode == 203;
    }
    // if we wrote any data to the save file, we try to commit the data to the real file.
    // if it actually got a proper file, we write it even if it was empty
    if (gotFile || wroteAnyData)
    {
        // ask validators for data consistency
        // we only do this for actual downloads, not 'your data is still the same' cache hits
        if(!finalizeAllValidators(reply))
            return Task::State::Failed;
        // nothing went wrong...
        if (!m_output_file->commit())
        {
            qCritical() << "Failed to commit changes to " << m_filename;
            m_output_file->cancelWriting();
            return Task::State::Failed;
        }
    }
    // then get rid of the save file
    m_output_file.reset();

    return finalizeCache(reply);
}

Task::State FileSink::finalizeCache(QNetworkReply &)
{
    return Task::State::Succeeded;
}

bool FileSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}
}
