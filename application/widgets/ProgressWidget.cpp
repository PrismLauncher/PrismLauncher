// Licensed under the Apache-2.0 license. See README.md for details.

#include "ProgressWidget.h"
		/*
		textBrowser->setHtml(QApplication::translate("JavaWizardPage",
			"<html><body>"
			"<p>MultiMC sends anonymous usage statistics on every start of the application. This helps us decide what platforms and issues to focus on.</p>"
			"<p>The data is processed by Google Analytics, see their <a href=\"https://support.google.com/analytics/answer/6004245?hl=en\">article on the matter</a>.</p>"
			"<p>The following data is collected:</p>"
			"<ul><li>A random unique ID of the MultiMC installation.<br />It is stored in the application settings (multimc.cfg).</li>"
			"<li>Anonymized IP address.<br />Last octet is set to 0 by Google and not stored.</li>"
			"<li>MultiMC version.</li>"
			"<li>Operating system name, version and architecture.</li>"
			"<li>CPU architecture (kernel architecture on linux).</li>"
			"<li>Size of system memory.</li>"
			"<li>Java version, architecture and memory settings.</li></ul>"
			"<p>The analytics will activate on next start, unless you disable them in the settings.</p>"
			"<p>If we change the tracked information, analytics won't be active for the first run and you will see this page again.</p></body></html>", Q_NULLPTR));
		checkBox->setText(QApplication::translate("JavaWizardPage", "Enable Analytics", Q_NULLPTR));
		*/
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QEventLoop>

#include "tasks/Task.h"

ProgressWidget::ProgressWidget(QWidget *parent)
	: QWidget(parent)
{
	m_label = new QLabel(this);
	m_label->setWordWrap(true);
	m_bar = new QProgressBar(this);
	m_bar->setMinimum(0);
	m_bar->setMaximum(100);
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_label);
	layout->addWidget(m_bar);
	layout->addStretch();
	setLayout(layout);
}

void ProgressWidget::start(std::shared_ptr<Task> task)
{
	if (m_task)
	{
		disconnect(m_task.get(), 0, this, 0);
	}
	m_task = task;
	connect(m_task.get(), &Task::finished, this, &ProgressWidget::handleTaskFinish);
	connect(m_task.get(), &Task::status, this, &ProgressWidget::handleTaskStatus);
	connect(m_task.get(), &Task::progress, this, &ProgressWidget::handleTaskProgress);
	connect(m_task.get(), &Task::destroyed, this, &ProgressWidget::taskDestroyed);
	if (!m_task->isRunning())
	{
		QMetaObject::invokeMethod(m_task.get(), "start", Qt::QueuedConnection);
	}
}
bool ProgressWidget::exec(std::shared_ptr<Task> task)
{
	QEventLoop loop;
	connect(task.get(), &Task::finished, &loop, &QEventLoop::quit);
	start(task);
	if (task->isRunning())
	{
		loop.exec();
	}
	return task->successful();
}

void ProgressWidget::handleTaskFinish()
{
	if (!m_task->successful())
	{
		m_label->setText(m_task->failReason());
	}
}
void ProgressWidget::handleTaskStatus(const QString &status)
{
	m_label->setText(status);
}
void ProgressWidget::handleTaskProgress(qint64 current, qint64 total)
{
	m_bar->setMaximum(total);
	m_bar->setValue(current);
}
void ProgressWidget::taskDestroyed()
{
	m_task = nullptr;
}
