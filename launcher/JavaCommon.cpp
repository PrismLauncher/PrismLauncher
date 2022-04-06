#include "JavaCommon.h"
#include "ui/dialogs/CustomMessageBox.h"
#include <MMCStrings.h>

bool JavaCommon::checkJVMArgs(QString jvmargs, QWidget *parent)
{
    if (jvmargs.contains("-XX:PermSize=") || jvmargs.contains(QRegExp("-Xm[sx]"))
        || jvmargs.contains("-XX-MaxHeapSize") || jvmargs.contains("-XX:InitialHeapSize"))
    {
        auto warnStr = QObject::tr(
            "You tried to manually set a JVM memory option (using \"-XX:PermSize\", \"-XX-MaxHeapSize\", \"-XX:InitialHeapSize\", \"-Xmx\" or \"-Xms\").\n"
            "There are dedicated boxes for these in the settings (Java tab, in the Memory group at the top).\n"
            "This message will be displayed until you remove them from the JVM arguments.");
        CustomMessageBox::selectable(
            parent, QObject::tr("JVM arguments warning"),
            warnStr,
            QMessageBox::Warning)->exec();
        return false;
    }
    // block lunacy with passing required version to the JVM
    if (jvmargs.contains(QRegExp("-version:.*"))) {
        auto warnStr = QObject::tr(
            "You tried to pass required java version argument to the JVM (using \"-version=xxx\"). This is not safe and will not be allowed.\n"
            "This message will be displayed until you remove this from the JVM arguments.");
        CustomMessageBox::selectable(
            parent, QObject::tr("JVM arguments warning"),
            warnStr,
            QMessageBox::Warning)->exec();
        return false;
    }
    return true;
}

void JavaCommon::javaWasOk(QWidget *parent, JavaCheckResult result)
{
    QString text;
    text += QObject::tr("Java test succeeded!<br />Platform reported: %1<br />Java version "
        "reported: %2<br />Java vendor "
        "reported: %3<br />").arg(result.realPlatform, result.javaVersion.toString(), result.javaVendor);
    if (result.errorLog.size())
    {
        auto htmlError = result.errorLog;
        htmlError.replace('\n', "<br />");
        text += QObject::tr("<br />Warnings:<br /><font color=\"orange\">%1</font>").arg(htmlError);
    }
    CustomMessageBox::selectable(parent, QObject::tr("Java test success"), text, QMessageBox::Information)->show();
}

void JavaCommon::javaArgsWereBad(QWidget *parent, JavaCheckResult result)
{
    auto htmlError = result.errorLog;
    QString text;
    htmlError.replace('\n', "<br />");
    text += QObject::tr("The specified Java binary didn't work with the arguments you provided:<br />");
    text += QString("<font color=\"red\">%1</font>").arg(htmlError);
    CustomMessageBox::selectable(parent, QObject::tr("Java test failure"), text, QMessageBox::Warning)->show();
}

void JavaCommon::javaBinaryWasBad(QWidget *parent, JavaCheckResult result)
{
    QString text;
    text += QObject::tr(
        "The specified Java binary didn't work.<br />You should use the auto-detect feature, "
        "or set the path to the Java executable.<br />");
    CustomMessageBox::selectable(parent, QObject::tr("Java test failure"), text, QMessageBox::Warning)->show();
}

void JavaCommon::TestCheck::run()
{
    if (!JavaCommon::checkJVMArgs(m_args, m_parent))
    {
        emit finished();
        return;
    }
    checker.reset(new JavaChecker());
    connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
            SLOT(checkFinished(JavaCheckResult)));
    checker->m_path = m_path;
    checker->performCheck();
}

void JavaCommon::TestCheck::checkFinished(JavaCheckResult result)
{
    if (result.validity != JavaCheckResult::Validity::Valid)
    {
        javaBinaryWasBad(m_parent, result);
        emit finished();
        return;
    }
    checker.reset(new JavaChecker());
    connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
            SLOT(checkFinishedWithArgs(JavaCheckResult)));
    checker->m_path = m_path;
    checker->m_args = m_args;
    checker->m_minMem = m_minMem;
    checker->m_maxMem = m_maxMem;
    if (result.javaVersion.requiresPermGen())
    {
        checker->m_permGen = m_permGen;
    }
    checker->performCheck();
}

void JavaCommon::TestCheck::checkFinishedWithArgs(JavaCheckResult result)
{
    if (result.validity == JavaCheckResult::Validity::Valid)
    {
        javaWasOk(m_parent, result);
        emit finished();
        return;
    }
    javaArgsWereBad(m_parent, result);
    emit finished();
}

