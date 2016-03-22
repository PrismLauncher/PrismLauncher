#include "JavaCommon.h"
#include "dialogs/CustomMessageBox.h"
#include <MMCStrings.h>

bool JavaCommon::checkJVMArgs(QString jvmargs, QWidget *parent)
{
	if (jvmargs.contains("-XX:PermSize=") || jvmargs.contains(QRegExp("-Xm[sx]")))
	{
		CustomMessageBox::selectable(
			parent, QObject::tr("JVM arguments warning"),
			QObject::tr("You tried to manually set a JVM memory option (using "
						" \"-XX:PermSize\", \"-Xmx\" or \"-Xms\") - there"
						" are dedicated boxes for these in the settings (Java"
						" tab, in the Memory group at the top).\n"
						"Your manual settings will be overridden by the"
						" dedicated options.\n"
						"This message will be displayed until you remove them"
						" from the JVM arguments."),
			QMessageBox::Warning)->exec();
		return false;
	}
	return true;
}

void JavaCommon::TestCheck::javaWasOk(JavaCheckResult result)
{
	QString text;
	text += tr("Java test succeeded!<br />Platform reported: %1<br />Java version "
			   "reported: %2<br />").arg(result.realPlatform, result.javaVersion.toString());
	if (result.errorLog.size())
	{
		auto htmlError = result.errorLog;
		htmlError.replace('\n', "<br />");
		text += tr("<br />Warnings:<br /><font color=\"orange\">%1</font>").arg(htmlError);
	}
	CustomMessageBox::selectable(m_parent, tr("Java test success"), text,
								 QMessageBox::Information)->show();
}

void JavaCommon::TestCheck::javaArgsWereBad(JavaCheckResult result)
{
	auto htmlError = result.errorLog;
	QString text;
	htmlError.replace('\n', "<br />");
	text += tr("The specified java binary didn't work with the arguments you provided:<br />");
	text += QString("<font color=\"red\">%1</font>").arg(htmlError);
	CustomMessageBox::selectable(m_parent, tr("Java test failure"), text, QMessageBox::Warning)
		->show();
}

void JavaCommon::TestCheck::javaBinaryWasBad(JavaCheckResult result)
{
	QString text;
	text += tr(
		"The specified java binary didn't work.<br />You should use the auto-detect feature, "
		"or set the path to the java executable.<br />");
	CustomMessageBox::selectable(m_parent, tr("Java test failure"), text, QMessageBox::Warning)
		->show();
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
	if (!result.valid)
	{
		javaBinaryWasBad(result);
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
	if (result.valid)
	{
		javaWasOk(result);
		emit finished();
		return;
	}
	javaArgsWereBad(result);
	emit finished();
}
