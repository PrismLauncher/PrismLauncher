#include "FileResolvingTask.h"

#include "Json.h"
#include "modplatform/ModIndex.h"
#include "net/ApiDownload.h"
#include "net/ApiUpload.h"
#include "net/Upload.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"

Flame::FileResolvingTask::FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess)
    : m_network(network), m_toProcess(toProcess)
{}

bool Flame::FileResolvingTask::abort()
{
    bool aborted = true;
    if (m_dljob)
        aborted &= m_dljob->abort();
    if (m_checkJob)
        aborted &= m_checkJob->abort();
    return aborted ? Task::abort() : false;
}

void Flame::FileResolvingTask::executeTask()
{
    if (m_toProcess.files.isEmpty()) {  // no file to resolve so leave it empty and emit success immediately
        emitSucceeded();
        return;
    }
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 3);
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

    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_dljob.get(), &NetJob::finished, this, [this, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        netJobFinished();
    });
    connect(m_dljob.get(), &NetJob::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_dljob.get(), &NetJob::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_dljob.get(), &NetJob::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_dljob.get(), &NetJob::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    setProgress(1, 3);
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
                QObject::connect(dl.get(), &Net::ApiDownload::succeeded, [&out]() { out.resolved = true; });

                m_checkJob->addNetAction(dl);
                blockedProjects.insert(&out, output);
            }
        }
    }
    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(m_checkJob.get(), &NetJob::finished, this, [this, step_progress]() {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        modrinthCheckFinished();
    });
    connect(m_checkJob.get(), &NetJob::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(m_checkJob.get(), &NetJob::stepProgress, this, &FileResolvingTask::propagateStepProgress);
    connect(m_checkJob.get(), &NetJob::progress, this, [this, step_progress](qint64 current, qint64 total) {
        qDebug() << "Resolve slug progress" << current << total;
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(m_checkJob.get(), &NetJob::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });

    m_checkJob->start();
}

void Flame::FileResolvingTask::modrinthCheckFinished()
{
    setProgress(2, 3);
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
            QObject::connect(dl.get(), &Net::ApiDownload::succeeded, [block, index, output]() {
                auto mod = block->at(index);  // use the shared_ptr so it is captured and only freed when we are done
                auto json = QJsonDocument::fromJson(*output);
                auto base =
                    Json::requireString(Json::requireObject(Json::requireObject(Json::requireObject(json), "data"), "links"), "websiteUrl");
                auto link = QString("%1/download/%2").arg(base, QString::number(mod->fileId));
                mod->websiteUrl = link;
            });
            m_slugJob->addNetAction(dl);
            index++;
        }
        auto step_progress = std::make_shared<TaskStepProgress>();
        connect(m_slugJob.get(), &NetJob::succeeded, this, [this, step_progress]() {
            step_progress->state = TaskStepState::Succeeded;
            stepProgress(*step_progress);
            emitSucceeded();
        });
        connect(m_slugJob.get(), &NetJob::failed, this, [this, step_progress](QString reason) {
            step_progress->state = TaskStepState::Failed;
            stepProgress(*step_progress);
            emitFailed(reason);
        });
        connect(m_slugJob.get(), &NetJob::stepProgress, this, &FileResolvingTask::propagateStepProgress);
        connect(m_slugJob.get(), &NetJob::progress, this, [this, step_progress](qint64 current, qint64 total) {
            qDebug() << "Resolve slug progress" << current << total;
            step_progress->update(current, total);
            stepProgress(*step_progress);
        });
        connect(m_slugJob.get(), &NetJob::status, this, [this, step_progress](QString status) {
            step_progress->status = status;
            stepProgress(*step_progress);
        });

        m_slugJob->start();
    } else {
        emitSucceeded();
    }
}
