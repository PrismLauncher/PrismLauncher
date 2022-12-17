#pragma once

#include <QTextBrowser>

#include "QObjectPtr.h"

QT_BEGIN_NAMESPACE
class VariableSizedImageObject;
QT_END_NAMESPACE

/** This subclasses QTextBrowser to provide additional capabilities
 *  to it, like allowing for images to be shown.
 */
class ProjectDescriptionPage final : public QTextBrowser {
    Q_OBJECT

   public:
    ProjectDescriptionPage(QWidget* parent = nullptr);

    void setMetaEntry(QString entry);

   public slots:
    /** Flushes the current processing happening in the page.
     *
     *  Should be called when changing the page's content entirely, to
     *  prevent old tasks from changing the new content.
     */
    void flush();

   private:
    shared_qobject_ptr<VariableSizedImageObject> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_image_text_object;
};
