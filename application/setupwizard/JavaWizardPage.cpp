#include "JavaWizardPage.h"
#include <MultiMC.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <widgets/VersionSelectWidget.h>
#include <widgets/IconLabel.h>
#include <FileSystem.h>

#include <java/JavaInstall.h>
#include <dialogs/CustomMessageBox.h>
#include <java/JavaUtils.h>
#include <sys.h>
#include <QFileDialog>


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
	connect(m_javaPathTextBox, &QLineEdit::textEdited, this, &JavaWizardPage::checkJavaPath);
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
	m_javaBrowseBtn->setText(QStringLiteral("..."));
	m_horizontalLayout->addWidget(m_javaBrowseBtn);

	m_javaStatusLabel = new IconLabel(this, badIcon, QSize(16, 16));
	m_horizontalLayout->addWidget(m_javaStatusLabel);

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
	m_minMemSpinBox->setMinimum(256);
	m_minMemSpinBox->setMaximum(m_availableMemory);
	m_minMemSpinBox->setSingleStep(128);
	m_minMemSpinBox->setValue(256);
	m_gridLayout_2->addWidget(m_minMemSpinBox, 0, 1, 1, 1);

	m_labelMaxMem = new QLabel(m_memoryGroupBox);
	m_labelMaxMem->setObjectName(QStringLiteral("labelMaxMem"));
	m_gridLayout_2->addWidget(m_labelMaxMem, 1, 0, 1, 1);

	m_maxMemSpinBox = new QSpinBox(m_memoryGroupBox);
	m_maxMemSpinBox->setObjectName(QStringLiteral("maxMemSpinBox"));
	m_maxMemSpinBox->setSuffix(QStringLiteral(" MB"));
	m_maxMemSpinBox->setMinimum(512);
	m_maxMemSpinBox->setMaximum(m_availableMemory);
	m_maxMemSpinBox->setSingleStep(128);
	m_maxMemSpinBox->setValue(1024);
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
	m_permGenSpinBox->setValue(128);
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
	m_minMemSpinBox->setValue(s->get("MinMemAlloc").toInt());
	m_maxMemSpinBox->setValue(s->get("MaxMemAlloc").toInt());
	m_permGenSpinBox->setValue(s->get("PermGen").toInt());
}

bool JavaWizardPage::validatePage()
{
	auto settings = MMC->settings();
	auto path = m_javaPathTextBox->text();
	switch(javaStatus)
	{
		case JavaStatus::Bad:
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
	int min = m_minMemSpinBox->value();
	int max = m_maxMemSpinBox->value();
	QObject *obj = sender();
	if (obj == m_minMemSpinBox)
	{
		if (min > max)
		{
			m_maxMemSpinBox->setValue(min);
		}
	}
	else if (obj == m_maxMemSpinBox)
	{
		if (min > max)
		{
			m_minMemSpinBox->setValue(max);
		}
	}
	checkJavaPath(m_javaPathTextBox->text());
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
	QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if(raw_path.isNull())
	{
		return;
	}
	QString cooked_path = FS::NormalizePath(raw_path);
	m_javaPathTextBox->setText(cooked_path);
	checkJavaPath(cooked_path);
}

void JavaWizardPage::setJavaStatus(JavaWizardPage::JavaStatus status)
{
	javaStatus = status;
	switch(javaStatus)
	{
		case JavaStatus::Good:
			m_javaStatusLabel->setIcon(goodIcon);
			break;
		case JavaStatus::Pending:
			m_javaStatusLabel->setIcon(yellowIcon);
			break;
		default:
		case JavaStatus::Bad:
			m_javaStatusLabel->setIcon(badIcon);
			break;
	}
}

void JavaWizardPage::checkJavaPath(const QString &path)
{
	auto realPath = FS::ResolveExecutable(path);
	if(realPath.isNull())
	{
		setJavaStatus(JavaStatus::Bad);
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
	if(result.valid)
	{
		setJavaStatus(JavaStatus::Good);
	}
	else
	{
		setJavaStatus(JavaStatus::Bad);
	}
	m_checker.reset();
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
}
