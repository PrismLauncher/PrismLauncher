#include "JavaSettingsWidget.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QFileDialog>

#include <sys.h>

#include "JavaCommon.h"
#include "java/JavaInstall.h"
#include "java/JavaUtils.h"
#include "FileSystem.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/widgets/VersionSelectWidget.h"

#include "Application.h"
#include "BuildConfig.h"

JavaSettingsWidget::JavaSettingsWidget(QWidget* parent) : QWidget(parent)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_availableMemory = Sys::getSystemRam() / Sys::mebibyte;

    goodIcon = APPLICATION->getThemedIcon("status-good");
    yellowIcon = APPLICATION->getThemedIcon("status-yellow");
    badIcon = APPLICATION->getThemedIcon("status-bad");
    setupUi();

    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox, SIGNAL(valueChanged(int)), this, SLOT(memoryValueChanged(int)));
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget, &VersionSelectWidget::selectedVersionChanged, this, &JavaSettingsWidget::javaVersionSelected);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn, &QPushButton::clicked, this, &JavaSettingsWidget::on_javaBrowseBtn_clicked);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox, &QLineEdit::textEdited, this, &JavaSettingsWidget::javaPathEdited);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn, &QToolButton::clicked, this, &JavaSettingsWidget::on_javaStatusBtn_clicked);
}

void JavaSettingsWidget::setupUi()
{
    setObjectName(QStringLiteral("javaSettingsWidget"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout = new QVBoxLayout(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget = new VersionSelectWidget(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout = new QHBoxLayout();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox = new QLineEdit(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->setObjectName(QStringLiteral("javaPathTextBox"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn = new QPushButton(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn->setObjectName(QStringLiteral("javaBrowseBtn"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn = new QToolButton(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn->setIcon(yellowIcon);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->addLayout(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox = new QGroupBox(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox->setObjectName(QStringLiteral("memoryGroupBox"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2 = new QGridLayout(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->setColumnStretch(0, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem = new QLabel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem->setObjectName(QStringLiteral("labelMinMem"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem, 0, 0, 1, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox = new QSpinBox(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setObjectName(QStringLiteral("minMemSpinBox"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setSuffix(QStringLiteral(" MiB"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setMinimum(128);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setMaximum(1048576);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setSingleStep(128);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem->setBuddy(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox, 0, 1, 1, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem = new QLabel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem->setObjectName(QStringLiteral("labelMaxMem"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem, 1, 0, 1, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox = new QSpinBox(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setObjectName(QStringLiteral("maxMemSpinBox"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setSuffix(QStringLiteral(" MiB"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setMinimum(128);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setMaximum(1048576);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setSingleStep(128);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem->setBuddy(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox, 1, 1, 1, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon = new QLabel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->setObjectName(QStringLiteral("labelMaxMemIcon"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon, 1, 2, 1, 1);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen = new QLabel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen->setObjectName(QStringLiteral("labelPermGen"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen->setText(QStringLiteral("PermGen:"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen, 2, 0, 1, 1);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen->setVisible(false);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox = new QSpinBox(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setObjectName(QStringLiteral("permGenSpinBox"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setSuffix(QStringLiteral(" MiB"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setMinimum(64);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setMaximum(1048576);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setSingleStep(8);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox, 2, 1, 1, 1);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setVisible(false);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox);

    retranslate();
}

void JavaSettingsWidget::initialize()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->initialize(APPLICATION->javalist().get());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->setResizeOn(2);
    auto s = APPLICATION->settings();
    // Memory
    observedMinMemory = s->get("MinMemAlloc").toInt();
    observedMaxMemory = s->get("MaxMemAlloc").toInt();
    observedPermGenMemory = s->get("PermGen").toInt();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setValue(observedMinMemory);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setValue(observedMaxMemory);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setValue(observedPermGenMemory);
    updateThresholds();
}

void JavaSettingsWidget::refresh()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget->loadList();
}

JavaSettingsWidget::ValidationStatus JavaSettingsWidget::validate()
{
    switch(javaStatus)
    {
        default:
        case JavaStatus::NotSet:
        case JavaStatus::DoesNotExist:
        case JavaStatus::DoesNotStart:
        case JavaStatus::ReturnedInvalidData:
        {
            int button = CustomMessageBox::selectable(
                this,
                tr("No Java version selected"),
                tr("You didn't select a Java version or selected something that doesn't work.\n"
                    "%1 will not be able to start Minecraft.\n"
                    "Do you wish to proceed without any Java?"
                    "\n\n"
                    "You can change the Java version in the settings later.\n"
                ).arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                QMessageBox::Warning,
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::NoButton
            )->exec();
            if(button == QMessageBox::No)
            {
                return ValidationStatus::Bad;
            }
            return ValidationStatus::JavaBad;
        }
        break;
        case JavaStatus::Pending:
        {
            return ValidationStatus::Bad;
        }
        case JavaStatus::Good:
        {
            return ValidationStatus::AllOK;
        }
    }
}

QString JavaSettingsWidget::javaPath() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->text();
}

int JavaSettingsWidget::maxHeapSize() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->value();
}

int JavaSettingsWidget::minHeapSize() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->value();
}

bool JavaSettingsWidget::permGenEnabled() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->isVisible();
}

int JavaSettingsWidget::permGenSize() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->value();
}

void JavaSettingsWidget::memoryValueChanged(int)
{
    bool actuallyChanged = false;
    unsigned int min = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->value();
    unsigned int max = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->value();
    unsigned int permgen = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->value();
    QObject *obj = sender();
    if (obj == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox && min != observedMinMemory)
    {
        observedMinMemory = min;
        actuallyChanged = true;
        if (min > max)
        {
            observedMaxMemory = min;
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setValue(min);
        }
    }
    else if (obj == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox && max != observedMaxMemory)
    {
        observedMaxMemory = max;
        actuallyChanged = true;
        if (min > max)
        {
            observedMinMemory = max;
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setValue(max);
        }
    }
    else if (obj == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox && permgen != observedPermGenMemory)
    {
        observedPermGenMemory = permgen;
        actuallyChanged = true;
    }
    if(actuallyChanged)
    {
        checkJavaPathOnEdit(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->text());
        updateThresholds();
    }
}

void JavaSettingsWidget::javaVersionSelected(BaseVersion::Ptr version)
{
    auto java = std::dynamic_pointer_cast<JavaInstall>(version);
    if(!java)
    {
        return;
    }
    auto visible = java->id.requiresPermGen();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen->setVisible(visible);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setVisible(visible);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->setText(java->path);
    checkJavaPath(java->path);
}

void JavaSettingsWidget::on_javaBrowseBtn_clicked()
{
    QString filter;
#if defined Q_OS_WIN32
    filter = "Java (javaw.exe)";
#else
    filter = "Java (java)";
#endif
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"), QString(), filter);
    if(raw_path.isEmpty())
    {
        return;
    }
    QString cooked_path = FS::NormalizePath(raw_path);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->setText(cooked_path);
    checkJavaPath(cooked_path);
}

void JavaSettingsWidget::on_javaStatusBtn_clicked()
{
    QString text;
    bool failed = false;
    switch(javaStatus)
    {
        case JavaStatus::NotSet:
            checkJavaPath(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox->text());
            return;
        case JavaStatus::DoesNotExist:
            text += QObject::tr("The specified file either doesn't exist or is not a proper executable.");
            failed = true;
            break;
        case JavaStatus::DoesNotStart:
        {
            text += QObject::tr("The specified Java binary didn't start properly.<br />");
            auto htmlError = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result.errorLog;
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
            text += QObject::tr("The specified Java binary returned unexpected results:<br />");
            auto htmlOut = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result.outLog;
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
                "reported: %2<br />").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result.realPlatform, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result.javaVersion.toString());
            break;
        case JavaStatus::Pending:
            // TODO: abort here?
            return;
    }
    CustomMessageBox::selectable(
        this,
        failed ? QObject::tr("Java test failure") : QObject::tr("Java test success"),
        text,
        failed ? QMessageBox::Critical : QMessageBox::Information
    )->show();
}

void JavaSettingsWidget::setJavaStatus(JavaSettingsWidget::JavaStatus status)
{
    javaStatus = status;
    switch(javaStatus)
    {
        case JavaStatus::Good:
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn->setIcon(goodIcon);
            break;
        case JavaStatus::NotSet:
        case JavaStatus::Pending:
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn->setIcon(yellowIcon);
            break;
        default:
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn->setIcon(badIcon);
            break;
    }
}

void JavaSettingsWidget::javaPathEdited(const QString& path)
{
    checkJavaPathOnEdit(path);
}

void JavaSettingsWidget::checkJavaPathOnEdit(const QString& path)
{
    auto realPath = FS::ResolveExecutable(path);
    QFileInfo pathInfo(realPath);
    if (pathInfo.baseName().toLower().contains("java"))
    {
        checkJavaPath(path);
    }
    else
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker)
        {
            setJavaStatus(JavaStatus::NotSet);
        }
    }
}

void JavaSettingsWidget::checkJavaPath(const QString &path)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker)
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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker.reset(new JavaChecker());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_path = path;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMem = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->value();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMem = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->value();
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->isVisible())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGen = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->value();
    }
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker.get(), &JavaChecker::checkFinished, this, &JavaSettingsWidget::checkFinished);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker->performCheck();
}

void JavaSettingsWidget::checkFinished(JavaCheckResult result)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result = result;
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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker.reset();
    if(!queuedCheck.isNull())
    {
        checkJavaPath(queuedCheck);
        queuedCheck.clear();
    }
}

void JavaSettingsWidget::retranslate()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox->setTitle(tr("Memory"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox->setToolTip(tr("The maximum amount of memory Minecraft is allowed to use."));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem->setText(tr("Minimum memory allocation:"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem->setText(tr("Maximum memory allocation:"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox->setToolTip(tr("The amount of memory Minecraft is started with."));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox->setToolTip(tr("The amount of memory available to store loaded Java classes."));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn->setText(tr("Browse"));
}

void JavaSettingsWidget::updateThresholds()
{
    QString iconName;

    if (observedMaxMemory >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_availableMemory) {
        iconName = "status-bad";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation exceeds your system memory capacity."));
    } else if (observedMaxMemory > (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_availableMemory * 0.9)) {
        iconName = "status-yellow";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation approaches your system memory capacity."));
    } else {
        iconName = "status-good";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->setToolTip("");
    }

    {
        auto height = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->fontInfo().pixelSize();
        QIcon icon = APPLICATION->getThemedIcon(iconName);
        QPixmap pix = icon.pixmap(height, height);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon->setPixmap(pix);
    }
}
