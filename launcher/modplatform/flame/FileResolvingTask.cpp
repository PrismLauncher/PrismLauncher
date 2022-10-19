#include "FileResolvingTask.h"

#include "Json.h"
#include "net/Upload.h"

Flame::FileResolvingTask::FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{}

bool Flame::FileResolvingTask::abort()
{
    bool aborted = true;
    if (m_dljob)
        aborted &= m_dljob->abort();
    return aborted ? Task::abort() : false;
}

void Flame::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 3);
    m_dljob = new NetJob("Mod id resolver", m_network);
    result.reset(new QByteArray());
    //build json data to send
    QJsonObject object;

    object["fileIds"] = QJsonArray::fromVariantList(std::accumulate(m_toProcess.files.begin(), m_toProcess.files.end(), QVariantList(), [](QVariantList& l, const File& s) {
        l.push_back(s.fileId);
        return l;
    }));
    QByteArray data = Json::toText(object);
    auto dl = Net::Upload::makeByteArray(QUrl("https://api.curseforge.com/v1/mods/files"), result.get(), data);
    m_dljob->addNetAction(dl);
    connect(m_dljob.get(), &NetJob::finished, this, &Flame::FileResolvingTask::netJobFinished);
    m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    setProgress(1, 3);
    int index = 0;
    // job to check modrinth for blocked projects
    auto job = new NetJob("Modrinth check", m_network);
    blockedProjects = QMap<File *,QByteArray *>();
    auto doc = Json::requireDocument(*result);
    auto array = Json::requireArray(doc.object()["data"]);
    for (QJsonValueRef file : array) {
        auto fileid = Json::requireInteger(Json::requireObject(file)["id"]);
        auto& out = m_toProcess.files[fileid];
        try {
           out.parseFromObject(Json::requireObject(file));
        } catch (const JSONValidationError& e) {
            qCDebug(LAUNCHER_LOG) << "Blocked mod on curseforge" << out.fileName;
            auto hash = out.hash;
            if(!hash.isEmpty()) {
                auto url = QString("https://api.modrinth.com/v2/version_file/%1?algorithm=sha1").arg(hash);
                auto output = new QByteArray();
                auto dl = Net::Download::makeByteArray(QUrl(url), output);
                QObject::connect(dl.get(), &Net::Download::succeeded, [&out]() {
                    out.resolved = true;
                });

                job->addNetAction(dl);
                blockedProjects.insert(&out, output);
            }
        }
        index++;
    }
    connect(job, &NetJob::finished, this, &Flame::FileResolvingTask::modrinthCheckFinished);

    job->start();
}

void Flame::FileResolvingTask::modrinthCheckFinished() {
    setProgress(2, 3);
    qCDebug(LAUNCHER_LOG) << "Finished with blocked mods : " << blockedProjects.size();

    for (auto it = blockedProjects.keyBegin(); it != blockedProjects.keyEnd(); it++) {
        auto &out = *it;
        auto bytes = blockedProjects[out];
        if (!out->resolved) {
            delete bytes;
            continue;
        }
        QJsonDocument doc = QJsonDocument::fromJson(*bytes);
        auto obj = doc.object();
        auto array = Json::requireArray(obj,"files");
        for (auto file: array) {
            auto fileObj = Json::requireObject(file);
            auto primary = Json::requireBoolean(fileObj,"primary");
            if (primary) {
                out->url = Json::requireUrl(fileObj,"url");
                qCDebug(LAUNCHER_LOG) << "Found alternative on modrinth " << out->fileName;
                break;
            }
        }
        delete bytes;
    }
    //copy to an output list and filter out projects found on modrinth
    auto block = new QList<File *>();
    auto it = blockedProjects.keys();
    std::copy_if(it.begin(), it.end(), std::back_inserter(*block), [](File *f) {
        return !f->resolved;
    });
    //Display not found mods early
    if (!block->empty()) {
        //blocked mods found, we need the slug for displaying.... we need another job :D !
        auto slugJob = new NetJob("Slug Job", m_network);
        auto slugs = QVector<QByteArray>(block->size());
        auto index = 0;
        for (auto fileInfo: *block) {
            auto projectId = fileInfo->projectId;
            slugs[index] = QByteArray();
            auto url = QString("https://api.curseforge.com/v1/mods/%1").arg(projectId);
            auto dl = Net::Download::makeByteArray(url, &slugs[index]);
            slugJob->addNetAction(dl);
            index++;
        }
        connect(slugJob, &NetJob::succeeded, this, [slugs, this, slugJob, block]() {
            slugJob->deleteLater();
            auto index = 0;
            for (const auto &slugResult: slugs) {
                auto json = QJsonDocument::fromJson(slugResult);
                auto base = Json::requireString(Json::requireObject(Json::requireObject(Json::requireObject(json),"data"),"links"),
                        "websiteUrl");
                auto mod = block->at(index);
                auto link = QString("%1/download/%2").arg(base, QString::number(mod->fileId));
                mod->websiteUrl = link;
                index++;
            }
            emitSucceeded();
        });
        slugJob->start();
    } else {
        emitSucceeded();
    }
}
