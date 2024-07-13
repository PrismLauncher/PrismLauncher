#include "FileResolvingTask.h"

#include "Json.h"
#include "modplatform/ModIndex.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/Upload.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "tasks/Task.h"

Flame::FileResolvingTask::FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{
    setCapabilities(Capability::Killable);
}

bool Flame::FileResolvingTask::doAbort()
{
    bool aborted = true;
    if (m_dljob)
        aborted &= m_dljob->abort();
    if (m_checkJob)
        aborted &= m_checkJob->abort();
    return aborted;
}

void Flame::FileResolvingTask::executeTask()
{
    if (m_toProcess.files.isEmpty()) {  // no file to resolve so leave it empty and emit success immediately
        emitSucceeded();
        return;
    }
    setStatus(tr("Resolving mod IDs..."));
    setProgressTotal(3);
    m_dljob.reset(new NetJob("Mod id resolver", m_network));
    result.reset(new QByteArray());
    // build json data to send
    QJsonObject object;

    object["fileIds"] = QJsonArray::fromVariantList(
        std::accumulate(m_toProcess.files.begin(), m_toProcess.files.end(), QVariantList(), [](QVariantList& l, const File& s) {
            l.push_back(s.fileId);
            return l;
        }));
    QByteArray data = Json::toText(object);
    auto dl = Net::ApiUpload::makeByteArray(QUrl("https://api.curseforge.com/v1/mods/files"), result, data);
    m_dljob->addNetAction(dl);

    connect(m_dljob.get(), &TaskV2::finished, this, &FileResolvingTask::netJobFinished);

    m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    setProgress(1);
    // job to check modrinth for blocked projects
    m_checkJob.reset(new NetJob("Modrinth check", m_network));
    blockedProjects = QMap<File*, std::shared_ptr<QByteArray>>();

    QJsonDocument doc;
    QJsonArray array;

    try {
        doc = Json::requireDocument(*result);
        array = Json::requireArray(doc.object()["data"]);
    } catch (Json::JsonException& e) {
        qCritical() << "Non-JSON data returned from the CF API";
        qCritical() << e.cause();

        emitFailed(tr("Invalid data returned from the API."));

        return;
    }

    for (QJsonValueRef file : array) {
        auto fileid = Json::requireInteger(Json::requireObject(file)["id"]);
        auto& out = m_toProcess.files[fileid];
        try {
            out.parseFromObject(Json::requireObject(file));
        } catch ([[maybe_unused]] const JSONValidationError& e) {
            qDebug() << "Blocked mod on curseforge" << out.fileName;
            auto hash = out.hash;
            if (!hash.isEmpty()) {
                auto url = QString("https://api.modrinth.com/v2/version_file/%1?algorithm=sha1").arg(hash);
                auto output = std::make_shared<QByteArray>();
                auto dl = Net::ApiDownload::makeByteArray(QUrl(url), output);
                QObject::connect(dl.get(), &TaskV2::finished, [&out](TaskV2* t) { out.resolved = t->wasSuccessful(); });

                m_checkJob->addNetAction(dl);
                blockedProjects.insert(&out, output);
            }
        }
    }
    connect(m_checkJob.get(), &TaskV2::finished, this, &FileResolvingTask::modrinthCheckFinished);

    m_checkJob->start();
}

void Flame::FileResolvingTask::modrinthCheckFinished()
{
    setProgress(2);
    qDebug() << "Finished with blocked mods : " << blockedProjects.size();

    for (auto it = blockedProjects.keyBegin(); it != blockedProjects.keyEnd(); it++) {
        auto& out = *it;
        auto bytes = blockedProjects[out];
        if (!out->resolved) {
            continue;
        }

        QJsonDocument doc = QJsonDocument::fromJson(*bytes);
        auto obj = doc.object();
        auto file = Modrinth::loadIndexedPackVersion(obj);

        // If there's more than one mod loader for this version, we can't know for sure
        // which file is relative to each loader, so it's best to not use any one and
        // let the user download it manually.
        if (!file.loaders || hasSingleModLoaderSelected(file.loaders)) {
            out->url = file.downloadUrl;
            qDebug() << "Found alternative on modrinth " << out->fileName;
        } else {
            out->resolved = false;
        }
    }
    // copy to an output list and filter out projects found on modrinth
    auto block = std::make_shared<QList<File*>>();
    auto it = blockedProjects.keys();
    std::copy_if(it.begin(), it.end(), std::back_inserter(*block), [](File* f) { return !f->resolved; });
    // Display not found mods early
    if (!block->empty()) {
        // blocked mods found, we need the slug for displaying.... we need another job :D !
        m_slugJob.reset(new NetJob("Slug Job", m_network));
        int index = 0;
        for (auto mod : *block) {
            auto projectId = mod->projectId;
            auto output = std::make_shared<QByteArray>();
            auto url = QString("https://api.curseforge.com/v1/mods/%1").arg(projectId);
            auto dl = Net::ApiDownload::makeByteArray(url, output);
            qDebug() << "Fetching url slug for file:" << mod->fileName;
            QObject::connect(dl.get(), &TaskV2::finished, [block, index, output](TaskV2* t) {
                if (t->wasSuccessful()) {
                    auto mod = block->at(index);  // use the shared_ptr so it is captured and only freed when we are done
                    auto json = QJsonDocument::fromJson(*output);
                    auto base = Json::requireString(Json::requireObject(Json::requireObject(Json::requireObject(json), "data"), "links"),
                                                    "websiteUrl");
                    auto link = QString("%1/download/%2").arg(base, QString::number(mod->fileId));
                    mod->websiteUrl = link;
                }
            });
            m_slugJob->addNetAction(dl);
            index++;
        }

        connect(m_slugJob.get(), &TaskV2::finished, this, [this](TaskV2* t) {
            if (t->wasSuccessful()) {
                emitSucceeded();
            } else {
                emitFailed(t->failReason());
            }
        });
        m_slugJob->start();
    } else {
        emitSucceeded();
    }
}
