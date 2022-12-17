#pragma once

#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <memory>

struct ScreenShot {
    using Ptr = std::shared_ptr<ScreenShot>;

    ScreenShot(QFileInfo file) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file = file;
    }
    QFileInfo hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_imgurId;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_imgurDeleteHash;
};
