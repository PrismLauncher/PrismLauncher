#include "InstanceSettingsPage.h"
#include <gui/dialogs/VersionSelectDialog.h>
#include "logic/NagUtils.h"
#include <logic/java/JavaVersionList.h>
#include "MultiMC.h"
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include "ui_InstanceSettingsPage.h"

QString InstanceSettingsPage::displayName() const
{
	return tr("Settings");
}

QIcon InstanceSettingsPage::icon() const
{
	return QIcon::fromTheme("settings");
}

QString InstanceSettingsPage::id() const
{
	return "settings";
}

InstanceSettingsPage::InstanceSettingsPage(BaseInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::InstanceSettingsPage), m_instance(inst)
{
	m_settings = &(inst->settings());
	ui->setupUi(this);
	loadSettings();
}

bool InstanceSettingsPage::shouldDisplay() const
{
	return !m_instance->isRunning();
}

InstanceSettingsPage::~InstanceSettingsPage()
{
	delete ui;
}

bool InstanceSettingsPage::apply()
{
	applySettings();
	return true;
}

void InstanceSettingsPage::applySettings()
{
	// Console
	bool console = ui->consoleSettingsBox->isChecked();
	m_settings->set("OverrideConsole", console);
	if (console)
	{
		m_settings->set("ShowConsole", ui->showConsoleCheck->isChecked());
		m_settings->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
	}
	else
	{
		m_settings->reset("ShowConsole");
		m_settings->reset("AutoCloseConsole");
	}

	// Window Size
	bool window = ui->windowSizeGroupBox->isChecked();
	m_settings->set("OverrideWindow", window);
	if (window)
	{
		m_settings->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
		m_settings->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
		m_settings->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
	}
	else
	{
		m_settings->reset("LaunchMaximized");
		m_settings->reset("MinecraftWinWidth");
		m_settings->reset("MinecraftWinHeight");
	}

	// Memory
	bool memory = ui->memoryGroupBox->isChecked();
	m_settings->set("OverrideMemory", memory);
	if (memory)
	{
		m_settings->set("MinMemAlloc", ui->minMemSpinBox->value());
		m_settings->set("MaxMemAlloc", ui->maxMemSpinBox->value());
		m_settings->set("PermGen", ui->permGenSpinBox->value());
	}
	else
	{
		m_settings->reset("MinMemAlloc");
		m_settings->reset("MaxMemAlloc");
		m_settings->reset("PermGen");
	}

	// Java Install Settings
	bool javaInstall = ui->javaSettingsGroupBox->isChecked();
	m_settings->set("OverrideJavaLocation", javaInstall);
	if (javaInstall)
	{
		m_settings->set("JavaPath", ui->javaPathTextBox->text());
	}
	else
	{
		m_settings->reset("JavaPath");
	}

	// Java arguments
	bool javaArgs = ui->javaArgumentsGroupBox->isChecked();
	m_settings->set("OverrideJavaArgs", javaArgs);
	if(javaArgs)
	{
		m_settings->set("JvmArgs", ui->jvmArgsTextBox->toPlainText().replace("\n", " "));
		NagUtils::checkJVMArgs(m_settings->get("JvmArgs").toString(), this->parentWidget());
	}
	else
	{
		m_settings->reset("JvmArgs");
	}

	// old generic 'override both' is removed.
	m_settings->reset("OverrideJava");

	// Custom Commands
	bool custcmd = ui->customCommandsGroupBox->isChecked();
	m_settings->set("OverrideCommands", custcmd);
	if (custcmd)
	{
		m_settings->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
		m_settings->set("PostExitCommand", ui->postExitCmdTextBox->text());
	}
	else
	{
		m_settings->reset("PreLaunchCommand");
		m_settings->reset("PostExitCommand");
	}
}

void InstanceSettingsPage::loadSettings()
{
	// Console
	ui->consoleSettingsBox->setChecked(m_settings->get("OverrideConsole").toBool());
	ui->showConsoleCheck->setChecked(m_settings->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(m_settings->get("AutoCloseConsole").toBool());

	// Window Size
	ui->windowSizeGroupBox->setChecked(m_settings->get("OverrideWindow").toBool());
	ui->maximizedCheckBox->setChecked(m_settings->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(m_settings->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(m_settings->get("MinecraftWinHeight").toInt());

	// Memory
	ui->memoryGroupBox->setChecked(m_settings->get("OverrideMemory").toBool());
	ui->minMemSpinBox->setValue(m_settings->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(m_settings->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(m_settings->get("PermGen").toInt());

	// Java Settings
	bool overrideJava = m_settings->get("OverrideJava").toBool();
	bool overrideLocation = m_settings->get("OverrideJavaLocation").toBool() || overrideJava;
	bool overrideArgs = m_settings->get("OverrideJavaArgs").toBool() || overrideJava;
	
	ui->javaSettingsGroupBox->setChecked(overrideLocation);
	ui->javaPathTextBox->setText(m_settings->get("JavaPath").toString());

	ui->javaArgumentsGroupBox->setChecked(overrideArgs);
	ui->jvmArgsTextBox->setPlainText(m_settings->get("JvmArgs").toString());

	// Custom Commands
	ui->customCommandsGroupBox->setChecked(m_settings->get("OverrideCommands").toBool());
	ui->preLaunchCmdTextBox->setText(m_settings->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(m_settings->get("PostExitCommand").toString());
}

void InstanceSettingsPage::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void InstanceSettingsPage::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}

void InstanceSettingsPage::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void InstanceSettingsPage::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform;
		QMessageBox::information(this, tr("Java test success"), text);
	}
	else
	{
		QMessageBox::warning(
			this, tr("Java test failure"),
			tr("The specified java binary didn't work. You should use the auto-detect feature, "
			   "or set the path to the java executable."));
	}
}
