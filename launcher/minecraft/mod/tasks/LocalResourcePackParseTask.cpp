#include "LocalResourcePackParseTask.h"

#include "FileSystem.h"
#include "Json.h"

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

LocalResourcePackParseTask::LocalResourcePackParseTask(int token, ResourcePack& rp)
    : Task(nullptr, false), m_token(token), m_resource_pack(rp)
{}

bool LocalResourcePackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalResourcePackParseTask::executeTask()
{
    switch (m_resource_pack.type()) {
        case ResourceType::FOLDER:
            processAsFolder();
            break;
        case ResourceType::ZIPFILE:
            processAsZip();
            break;
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            emitFailed(tr("Invalid type."));
    }

    if (isFinished())
        return;

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}

// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
void LocalResourcePackParseTask::processMCMeta(QByteArray&& raw_data)
{
    try {
        auto json_doc = QJsonDocument::fromJson(raw_data);
        auto pack_obj = Json::requireObject(json_doc.object(), "pack", {});

        m_resource_pack.setPackFormat(Json::ensureInteger(pack_obj, "pack_format", 0));
        m_resource_pack.setDescription(Json::ensureString(pack_obj, "description", ""));
    } catch (Json::JsonException& e) {
        qWarning() << "JsonException: " << e.what() << e.cause();
        emitFailed(tr("Failed to process .mcmeta file."));
    }
}

void LocalResourcePackParseTask::processAsFolder()
{
    QFileInfo mcmeta_file_info(FS::PathCombine(m_resource_pack.fileinfo().filePath(), "pack.mcmeta"));
    if (mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return;

        auto data = mcmeta_file.readAll();
        if (data.isEmpty() || data.isNull())
            return;

        processMCMeta(std::move(data));
    }
}

void LocalResourcePackParseTask::processAsZip()
{
    QuaZip zip(m_resource_pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("pack.mcmeta")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return;
        }

        auto data = file.readAll();

        processMCMeta(std::move(data));

        file.close();
        zip.close();
    }
}
