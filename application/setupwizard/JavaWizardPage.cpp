#include "JavaWizardPage.h"
#include <MultiMC.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <widgets/VersionSelectWidget.h>
#include <FileSystem.h>

#include <java/JavaInstall.h>
#include <dialogs/CustomMessageBox.h>
#include <java/JavaUtils.h>
#include <sys.h>
#include <QFileDialog>
#include <JavaCommon.h>


JavaWizardPage::JavaWizardPage(QWidget *parent)
	:BaseWizardPage(parent)
{
	m_availableMemory = Sys::getSystemRam() / (1024ull * 1024ull);

	goodIcon = MMC->getThemedIcon("status-good");
	yellowIcon = MMC->getThemedIcon("status-yellow");
	badIcon = MMC->getThemedIcon("status-bad");
	setupUi();

	connect(m_minMemSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
	connect(m_maxMemSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
	connect(m_permGenSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
	connect(m_versionWidget, &VersionSelectWidget::selectedVersionChanged, this, &JavaWizardPage::javaVersionSelected);
	connect(m_javaBrowseBtn, &QPushButton::clicked, this, &JavaWizardPage::on_javaBrowseBtn_clicked);
	connect(m_javaPathTextBox, &QLineEdit::textEdited, this, &JavaWizardPage::javaPathEdited);
	connect(m_javaStatusBtn, &QToolButton::clicked, this, &JavaWizardPage::on_javaStatusBtn_clicked);
}

void JavaWizardPage::setupUi()
{
	setObjectName(QStringLiteral("javaPage"));
	m_verticalLayout = new QVBoxLayout(this);
	m_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

	m_versionWidget = new VersionSelectWidget(MMC->javalist().get(), this);
	m_versionWidget->setResizeOn(2);
	m_verticalLayout->addWidget(m_versionWidget);

	m_horizontalLayout = new QHBoxLayout();
	m_horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
	m_javaPathTextBox = new QLineEdit(this);
	m_javaPathTextBox->setObjectName(QStringLiteral("javaPathTextBox"));

	m_horizontalLayout->addWidget(m_javaPathTextBox);

	m_javaBrowseBtn = new QPushButton(this);
	m_javaBrowseBtn->setObjectName(QStringLiteral("javaBrowseBtn"));
	/*
	QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy2.setHorizontalStretch(0);
	sizePolicy2.setVerticalStretch(0);
	sizePolicy2.setHeightForWidth(m_javaBrowseBtn->sizePolicy().hasHeightForWidth());
	m_javaBrowseBtn->setSizePolicy(sizePolicy2);
	m_javaBrowseBtn->setMaximumSize(QSize(28, 16777215));
	*/
	m_horizontalLayout->addWidget(m_javaBrowseBtn);

	m_javaStatusBtn = new QToolButton(this);
	m_javaStatusBtn->setIcon(yellowIcon);
	m_horizontalLayout->addWidget(m_javaStatusBtn);

	m_verticalLayout->addLayout(m_horizontalLayout);

	m_memoryGroupBox = new QGroupBox(this);
	m_memoryGroupBox->setObjectName(QStringLiteral("memoryGroupBox"));
	m_gridLayout_2 = new QGridLayout(m_memoryGroupBox);
	m_gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));

	m_labelMinMem = new QLabel(m_memoryGroupBox);
	m_labelMinMem->setObjectName(QStringLiteral("labelMinMem"));
	m_gridLayout_2->addWidget(m_labelMinMem, 0, 0, 1, 1);

	m_minMemSpinBox = new QSpinBox(m_memoryGroupBox);
	m_minMemSpinBox->setObjectName(QStringLiteral("minMemSpinBox"));
	m_minMemSpinBox->setSuffix(QStringLiteral(" MB"));
	m_minMemSpinBox->setMinimum(128);
	m_minMemSpinBox->setMaximum(m_availableMemory);
	m_minMemSpinBox->setSingleStep(128);
	m_labelMinMem->setBuddy(m_minMemSpinBox);
	m_gridLayout_2->addWidget(m_minMemSpinBox, 0, 1, 1, 1);

	m_labelMaxMem = new QLabel(m_memoryGroupBox);
	m_labelMaxMem->setObjectName(QStringLiteral("labelMaxMem"));
	m_gridLayout_2->addWidget(m_labelMaxMem, 1, 0, 1, 1);

	m_maxMemSpinBox = new QSpinBox(m_memoryGroupBox);
	m_maxMemSpinBox->setObjectName(QStringLiteral("maxMemSpinBox"));
	m_maxMemSpinBox->setSuffix(QStringLiteral(" MB"));
	m_maxMemSpinBox->setMinimum(128);
	m_maxMemSpinBox->setMaximum(m_availableMemory);
	m_maxMemSpinBox->setSingleStep(128);
	m_labelMaxMem->setBuddy(m_maxMemSpinBox);
	m_gridLayout_2->addWidget(m_maxMemSpinBox, 1, 1, 1, 1);

	m_labelPermGen = new QLabel(m_memoryGroupBox);
	m_labelPermGen->setObjectName(QStringLiteral("labelPermGen"));
	m_labelPermGen->setText(QStringLiteral("PermGen:"));
	m_gridLayout_2->addWidget(m_labelPermGen, 2, 0, 1, 1);
	m_labelPermGen->setVisible(false);

	m_permGenSpinBox = new QSpinBox(m_memoryGroupBox);
	m_permGenSpinBox->setObjectName(QStringLiteral("permGenSpinBox"));
	m_permGenSpinBox->setSuffix(QStringLiteral(" MB"));
	m_permGenSpinBox->setMinimum(64);
	m_permGenSpinBox->setMaximum(m_availableMemory);
	m_permGenSpinBox->setSingleStep(8);
	m_gridLayout_2->addWidget(m_permGenSpinBox, 2, 1, 1, 1);
	m_permGenSpinBox->setVisible(false);

	m_verticalLayout->addWidget(m_memoryGroupBox);

	retranslate();
}

void JavaWizardPage::refresh()
{
	m_versionWidget->loadList();
}

void JavaWizardPage::initializePage()
{
	m_versionWidget->initialize();
	auto s = MMC->settings();
	// Memory
	observedMinMemory = s->get("MinMemAlloc").toInt();
	observedMaxMemory = s->get("MaxMemAlloc").toInt();
	observedPermGenMemory = s->get("PermGen").toInt();
	m_minMemSpinBox->setValue(observedMinMemory);
	m_maxMemSpinBox->setValue(observedMaxMemory);
	m_permGenSpinBox->setValue(observedPermGenMemory);
}

bool JavaWizardPage::validatePage()
{
	auto settings = MMC->settings();
	auto path = m_javaPathTextBox->text();
	switch(javaStatus)
	{
		case JavaStatus::NotSet:
		case JavaStatus::DoesNotExist:
		case JavaStatus::DoesNotStart:
		case JavaStatus::ReturnedInvalidData:
		{
			int button = CustomMessageBox::selectable(
				this,
				tr("No Java version selected"),
				tr("You didn't select a Java version or selected something that doesn't work.\n"
					"MultiMC will not be able to start Minecraft.\n"
					"Do you wish to proceed without any Java?"
					"\n\n"
					"You can change the Java version in the settings later.\n"
				),
				QMessageBox::Warning,
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::NoButton
			)->exec();
			if(button == QMessageBox::No)
			{
				return false;
			}
		}
		break;
		case JavaStatus::Pending:
		{
			return false;
		}
		case JavaStatus::Good:
		{
			settings->set("JavaPath", path);
		}
	}

	// Memory
	auto s = MMC->settings();
	s->set("MinMemAlloc", m_minMemSpinBox->value());
	s->set("MaxMemAlloc", m_maxMemSpinBox->value());
	if (m_permGenSpinBox->isVisible())
	{
		s->set("PermGen", m_permGenSpinBox->value());
	}
	else
	{
		s->reset("PermGen");
	}
	return true;
}

bool JavaWizardPage::isRequired()
{
	QString currentHostName = QHostInfo::localHostName();
	QString oldHostName = MMC->settings()->get("LastHostname").toString();
	if (currentHostName != oldHostName)
	{
		MMC->settings()->set("LastHostname", currentHostName);
		return true;
	}
	QString currentJavaPath = MMC->settings()->get("JavaPath").toString();
	QString actualPath = FS::ResolveExecutable(currentJavaPath);
	if (actualPath.isNull())
	{
		return true;
	}
	return false;
}

bool JavaWizardPage::wantsRefreshButton()
{
	return true;
}

void JavaWizardPage::memoryValueChanged(int)
{
	bool actuallyChanged = false;
	int min = m_minMemSpinBox->value();
	int max = m_maxMemSpinBox->value();
	int permgen = m_permGenSpinBox->value();
	QObject *obj = sender();
	if (obj == m_minMemSpinBox && min != observedMinMemory)
	{
		observedMinMemory = min;
		actuallyChanged = true;
		if (min > max)
		{
			observedMaxMemory = min;
			m_maxMemSpinBox->setValue(min);
		}
	}
	else if (obj == m_maxMemSpinBox && max != observedMaxMemory)
	{
		observedMaxMemory = max;
		actuallyChanged = true;
		if (min > max)
		{
			observedMinMemory = max;
			m_minMemSpinBox->setValue(max);
		}
	}
	else if (obj == m_permGenSpinBox && permgen != observedPermGenMemory)
	{
		observedPermGenMemory = permgen;
		actuallyChanged = true;
	}
	if(actuallyChanged)
	{
		checkJavaPathOnEdit(m_javaPathTextBox->text());
	}
}

void JavaWizardPage::javaVersionSelected(BaseVersionPtr version)
{
	auto java = std::dynamic_pointer_cast<JavaInstall>(version);
	if(!java)
	{
		return;
	}
	auto visible = java->id.requiresPermGen();
	m_labelPermGen->setVisible(visible);
	m_permGenSpinBox->setVisible(visible);
	m_javaPathTextBox->setText(java->path);
	checkJavaPath(java->path);
}

void JavaWizardPage::on_javaBrowseBtn_clicked()
{
	QString filter;
#if defined Q_OS_WIN32
	filter = "Java (javaw.exe)";
#else
	filter = "Java (java)";
#endif
	QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"), QString(), filter);
	if(raw_path.isNull())
	{
		return;
	}
	QString cooked_path = FS::NormalizePath(raw_path);
	m_javaPathTextBox->setText(cooked_path);
	checkJavaPath(cooked_path);
}

void JavaWizardPage::on_javaStatusBtn_clicked()
{
	QString text;
	bool failed = false;
	switch(javaStatus)
	{
		case JavaStatus::NotSet:
			checkJavaPath(m_javaPathTextBox->text());
			return;
		case JavaStatus::DoesNotExist:
			text += QObject::tr("The specified file either doesn't exist or is not a proper executable.");
			failed = true;
			break;
		case JavaStatus::DoesNotStart:
		{
			text += QObject::tr("The specified java binary didn't start properly.<br />");
			auto htmlError = m_result.errorLog;
			if(!htmlError.isEmpty())
			{
				htmlError.replace('\n', "<br />");
				text += QString("<font color=\"red\">%1</font>").arg(htmlError);
			}
			failed = true;
			break;
		}
		case JavaStatus::ReturnedInvalidData:
		{
			text += QObject::tr("The specified java binary returned unexpected results:<br />");
			auto htmlOut = m_result.outLog;
			if(!htmlOut.isEmpty())
			{
				htmlOut.replace('\n', "<br />");
				text += QString("<font color=\"red\">%1</font>").arg(htmlOut);
			}
			failed = true;
			break;
		}
		case JavaStatus::Good:
			text += QObject::tr("Java test succeeded!<br />Platform reported: %1<br />Java version "
				"reported: %2<br />").arg(m_result.realPlatform, m_result.javaVersion.toString());
			break;
		case JavaStatus::Pending:
			// TODO: abort here?
			return;
	}
	CustomMessageBox::selectable(
		this,
		failed ? QObject::tr("Java test success") : QObject::tr("Java test failure"),
		text,
		failed ? QMessageBox::Critical : QMessageBox::Information
	)->show();
}

void JavaWizardPage::setJavaStatus(JavaWizardPage::JavaStatus status)
{
	javaStatus = status;
	switch(javaStatus)
	{
		case JavaStatus::Good:
			m_javaStatusBtn->setIcon(goodIcon);
			break;
		case JavaStatus::NotSet:
		case JavaStatus::Pending:
			m_javaStatusBtn->setIcon(yellowIcon);
			break;
		default:
			m_javaStatusBtn->setIcon(badIcon);
			break;
	}
}

void JavaWizardPage::javaPathEdited(const QString& path)
{
	checkJavaPathOnEdit(path);
}

void JavaWizardPage::checkJavaPathOnEdit(const QString& path)
{
	auto realPath = FS::ResolveExecutable(path);
	QFileInfo pathInfo(realPath);
	if (pathInfo.baseName().toLower().contains("java"))
	{
		checkJavaPath(path);
	}
	else
	{
		if(!m_checker)
		{
			setJavaStatus(JavaStatus::NotSet);
		}
	}
}

void JavaWizardPage::checkJavaPath(const QString &path)
{
	if(m_checker)
	{
		queuedCheck = path;
		return;
	}
	auto realPath = FS::ResolveExecutable(path);
	if(realPath.isNull())
	{
		setJavaStatus(JavaStatus::DoesNotExist);
		return;
	}
	setJavaStatus(JavaStatus::Pending);
	m_checker.reset(new JavaChecker());
	m_checker->m_path = path;
	m_checker->m_minMem = m_minMemSpinBox->value();
	m_checker->m_maxMem = m_maxMemSpinBox->value();
	if(m_permGenSpinBox->isVisible())
	{
		m_checker->m_permGen = m_permGenSpinBox->value();
	}
	connect(m_checker.get(), &JavaChecker::checkFinished, this, &JavaWizardPage::checkFinished);
	m_checker->performCheck();
}

void JavaWizardPage::checkFinished(JavaCheckResult result)
{
	m_result = result;
	switch(result.validity)
	{
		case JavaCheckResult::Validity::Valid:
		{
			setJavaStatus(JavaStatus::Good);
			break;
		}
		case JavaCheckResult::Validity::ReturnedInvalidData:
		{
			setJavaStatus(JavaStatus::ReturnedInvalidData);
			break;
		}
		case JavaCheckResult::Validity::Errored:
		{
			setJavaStatus(JavaStatus::DoesNotStart);
			break;
		}
	}
	m_checker.reset();
	if(!queuedCheck.isNull())
	{
		checkJavaPath(queuedCheck);
		queuedCheck.clear();
	}
}

void JavaWizardPage::retranslate()
{
	setTitle(QApplication::translate("JavaWizardPage", "Java", Q_NULLPTR));
	setSubTitle(QApplication::translate("JavaWizardPage", "You do not have a working Java set up yet or it went missing.\n"
		"Please select one of the following or browse for a java executable.", Q_NULLPTR));
	m_memoryGroupBox->setTitle(QApplication::translate("JavaPage", "Memory", Q_NULLPTR));
	m_maxMemSpinBox->setToolTip(QApplication::translate("JavaPage", "The maximum amount of memory Minecraft is allowed to use.", Q_NULLPTR));
	m_labelMinMem->setText(QApplication::translate("JavaPage", "Minimum memory allocation:", Q_NULLPTR));
	m_labelMaxMem->setText(QApplication::translate("JavaPage", "Maximum memory allocation:", Q_NULLPTR));
	m_minMemSpinBox->setToolTip(QApplication::translate("JavaPage", "The amount of memory Minecraft is started with.", Q_NULLPTR));
	m_permGenSpinBox->setToolTip(QApplication::translate("JavaPage", "The amount of memory available to store loaded Java classes.", Q_NULLPTR));
	m_javaBrowseBtn->setText(QApplication::translate("JavaPage", "Browse", Q_NULLPTR));
}
