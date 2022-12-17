#include "ProjectDescriptionPage.h"

#include "VariableSizedImageObject.h"

#include <QDebug>

ProjectDescriptionPage::ProjectDescriptionPage(QWidget* parent) : QTextBrowser(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object(new VariableSizedImageObject)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object->setParent(this);
    document()->documentLayout()->registerHandler(QTextFormat::ImageObject, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object.get());
}

void ProjectDescriptionPage::setMetaEntry(QString entry)
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object->setMetaEntry(entry);
}

void ProjectDescriptionPage::flush()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object->flush();
}
