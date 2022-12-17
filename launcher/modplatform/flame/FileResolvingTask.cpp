#include "FileResolvingTask.h"

#include "Json.h"
#include "net/Upload.h"

#include "modplatform/modrinth/ModrinthPackIndex.h"

Flame::FileResolvingTask::FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest& toProcess)
    : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network(network), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess(toProcess)
{}

bool Flame::FileResolvingTask::abort()
{
    bool aborted = true;
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob)
        aborted &= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob->abort();
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob)
        aborted &= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob->abort();
    return aborted ? Task::abort() : false;
}

void Flame::FileResolvingTask::executeTask()
{
    setStatus(tr("Resolving mod IDs..."));
    setProgress(0, 3);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob = new NetJob("Mod id resolver", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);
    result.reset(new QByteArray());
    //build json data to send
    QJsonObject object;

    object["fileIds"] = QJsonArray::fromVariantList(std::accumulate(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess.files.begin(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess.files.end(), QVariantList(), [](QVariantList& l, const File& s) {
        l.push_back(s.fileId);
        return l;
    }));
    QByteArray data = Json::toText(object);
    auto dl = Net::Upload::makeByteArray(QUrl("https://api.curseforge.com/v1/mods/files"), result.get(), data);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob->addNetAction(dl);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob.get(), &NetJob::finished, this, &Flame::FileResolvingTask::netJobFinished);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
    setProgress(1, 3);
    // job to check modrinth for blocked projects
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob = new NetJob("Modrinth check", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);
    blockedProjects = QMap<File *,QByteArray *>();

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
        auto& out = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess.files[fileid];
        try {
           out.parseFromObject(Json::requireObject(file));
        } catch (const JSONValidationError& e) {
            qDebug() << "Blocked mod on curseforge" << out.fileName;
            auto hash = out.hash;
            if(!hash.isEmpty()) {
                auto url = QString("https://api.modrinth.com/v2/version_file/%1?algorithm=sha1").arg(hash);
                auto output = new QByteArray();
                auto dl = Net::Download::makeByteArray(QUrl(url), output);
                QObject::connect(dl.get(), &Net::Download::succeeded, [&out]() {
                    out.resolved = true;
                });

                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob->addNetAction(dl);
                blockedProjects.insert(&out, output);
            }
        }
    }
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob.get(), &NetJob::finished, this, &Flame::FileResolvingTask::modrinthCheckFinished);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob->start();
}

void Flame::FileResolvingTask::modrinthCheckFinished() {
    setProgress(2, 3);
    qDebug() << "Finished with blocked mods : " << blockedProjects.size();

    for (auto it = blockedProjects.keyBegin(); it != blockedProjects.keyEnd(); it++) {
        auto &out = *it;
        auto bytes = blockedProjects[out];
        if (!out->resolved) {
            delete bytes;
            continue;
        }

        QJsonDocument doc = QJsonDocument::fromJson(*bytes);
        auto obj = doc.object();
        auto file = Modrinth::loadIndexedPackVersion(obj);

        // If there's more than one mod loader for this version, we can't know for sure
        // which file is relative to each loader, so it's best to not use any one and
        // let the user download it manually.
        if (file.loaders.size() <= 1) {
            out->url = file.downloadUrl;
            qDebug() << "Found alternative on modrinth " << out->fileName;
        } else {
            out->resolved = false;
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
        auto slugJob = new NetJob("Slug Job", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);
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
