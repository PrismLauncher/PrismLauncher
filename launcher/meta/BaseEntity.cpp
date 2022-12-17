/* Copyright 2015-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BaseEntity.h"

#include "net/Download.h"
#include "net/HttpMetaCache.h"
#include "net/NetJob.h"
#include "Json.h"

#include "BuildConfig.h"
#include "Application.h"

class ParsingValidator : public Net::Validator
{
public: /* con/des */
    ParsingValidator(Meta::BaseEntity *entity) : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entity(entity)
    {
    };
    virtual ~ParsingValidator()
    {
    };

public: /* methods */
    bool init(QNetworkRequest &) override
    {
        return true;
    }
    bool write(QByteArray & data) override
    {
        this->data.append(data);
        return true;
    }
    bool abort() override
    {
        return true;
    }
    bool validate(QNetworkReply &) override
    {
        auto fname = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entity->localFilename();
        try
        {
            auto doc = Json::requireDocument(data, fname);
            auto obj = Json::requireObject(doc, fname);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entity->parse(obj);
            return true;
        }
        catch (const Exception &e)
        {
            qWarning() << "Unable to parse response:" << e.cause();
            return false;
        }
    }

private: /* data */
    QByteArray data;
    Meta::BaseEntity *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_entity;
};

Meta::BaseEntity::~BaseEntity()
{
}

QUrl Meta::BaseEntity::url() const
{
    auto s = APPLICATION->settings();
    QString metaOverride = s->get("MetaURLOverride").toString();
    if(metaOverride.isEmpty())
    {
        return QUrl(BuildConfig.META_URL).resolved(localFilename());
    }
    else
    {
        return QUrl(metaOverride).resolved(localFilename());
    }
}

bool Meta::BaseEntity::loadLocalFile()
{
    const QString fname = QDir("meta").absoluteFilePath(localFilename());
    if (!QFile::exists(fname))
    {
        return false;
    }
    // TODO: check if the file has the expected checksum
    try
    {
        auto doc = Json::requireDocument(fname, fname);
        auto obj = Json::requireObject(doc, fname);
        parse(obj);
        return true;
    }
    catch (const Exception &e)
    {
        qDebug() << QString("Unable to parse file %1: %2").arg(fname, e.cause());
        // just make sure it's gone and we never consider it again.
        QFile::remove(fname);
        return false;
    }
}

void Meta::BaseEntity::load(Net::Mode loadType)
{
    // load local file if nothing is loaded yet
    if(!isLoaded())
    {
        if(loadLocalFile())
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loadStatus = LoadStatus::Local;
        }
    }
    // if we need remote update, run the update task
    if(loadType == Net::Mode::Offline || !shouldStartRemoteUpdate())
    {
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask = new NetJob(QObject::tr("Download of meta file %1").arg(localFilename()), APPLICATION->network());
    auto url = this->url();
    auto entry = APPLICATION->metacache()->resolveEntry("meta", localFilename());
    entry->setStale(true);
    auto dl = Net::Download::makeCached(url, entry);
    /*
     * The validator parses the file and loads it into the object.
     * If that fails, the file is not written to storage.
     */
    dl->addValidator(new ParsingValidator(this));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->addNetAction(dl);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateStatus = UpdateStatus::InProgress;
    QObject::connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.get(), &NetJob::succeeded, [&]()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loadStatus = LoadStatus::Remote;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateStatus = UpdateStatus::Succeeded;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.reset();
    });
    QObject::connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.get(), &NetJob::failed, [&]()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateStatus = UpdateStatus::Failed;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask.reset();
    });
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask->start();
}

bool Meta::BaseEntity::isLoaded() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loadStatus > LoadStatus::NotLoaded;
}

bool Meta::BaseEntity::shouldStartRemoteUpdate() const
{
    // TODO: version-locks and offline mode?
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateStatus != UpdateStatus::InProgress;
}

Task::Ptr Meta::BaseEntity::getCurrentTask()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateStatus == UpdateStatus::InProgress)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateTask;
    }
    return nullptr;
}
