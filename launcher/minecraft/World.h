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

#pragma once
#include <QFileInfo>
#include <QDateTime>
#include <optional>

struct GameType {
    GameType() = default;
    GameType (std::optional<int> original);

    QString toTranslatedString() const;
    QString toLogString() const;

    enum
    {
        Unknown = -1,
        Survival = 0,
        Creative,
        Adventure,
        Spectator
    } type = Unknown;
    std::optional<int> original;
};

class World
{
public:
    World(const QFileInfo &file);
    QString folderName() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_folderName;
    }
    QString name() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_actualName;
    }
    QString iconFile() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconFile;
    }
    int64_t bytes() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_size;
    }
    QDateTime lastPlayed() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lastPlayed;
    }
    GameType gameType() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gameType;
    }
    int64_t seed() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_randomSeed;
    }
    bool isValid() const
    {
        return is_valid;
    }
    bool isOnFS() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_containerFile.isDir();
    }
    QFileInfo container() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_containerFile;
    }
    // delete all the files of this world
    bool destroy();
    // replace this world with a copy of the other
    bool replace(World &with);
    // change the world's filesystem path (used by world lists for *MAGIC* purposes)
    void repath(const QFileInfo &file);
    // remove the icon file, if any
    bool resetIcon();

    bool rename(const QString &to);
    bool install(const QString &to, const QString &name= QString());

    // WEAK compare operator - used for replacing worlds
    bool operator==(const World &other) const;

private:
    void readFromZip(const QFileInfo &file);
    void readFromFS(const QFileInfo &file);
    void loadFromLevelDat(QByteArray data);

protected:

    QFileInfo hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_containerFile;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_containerOffsetPath;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_folderName;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_actualName;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconFile;
    QDateTime levelDatTime;
    QDateTime hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lastPlayed;
    int64_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_size;
    int64_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_randomSeed = 0;
    GameType hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gameType;
    bool is_valid = false;
};
