#include "JavaWizardPage.h"
#include "Application.h"
#include "config/GlobalConfig.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include "JavaCommon.h"

#include "ui/widgets/JavaWizardWidget.h"
#include "ui/widgets/VersionSelectWidget.h"

JavaWizardPage::JavaWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    setupUi();
}

void JavaWizardPage::setupUi()
{
    setObjectName(QStringLiteral("javaPage"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_java_widget = new JavaWizardWidget(this);
    layout->addWidget(m_java_widget);
    setLayout(layout);

    retranslate();
}

void JavaWizardPage::refresh()
{
    m_java_widget->refresh();
}

void JavaWizardPage::initializePage()
{
    m_java_widget->initialize();
}

bool JavaWizardPage::wantsRefreshButton()
{
    return true;
}

bool JavaWizardPage::validatePage()
{
    auto& conf = APPLICATION->config().update();
    auto result = m_java_widget->validate();
    conf.automaticJavaSwitch = m_java_widget->autoDetectJava();
    conf.automaticJavaDownload = m_java_widget->autoDownloadJava();
    conf.userAskedAboutAutomaticJavaDownload = true;

    if (result == JavaWizardWidget::ValidationStatus::Bad) {
        return false;
    }

    if (result == JavaWizardWidget::ValidationStatus::AllOK) {
        conf.javaInstallation.path = m_java_widget->javaPath();
    }

    // Memory
    conf.memory.minAlloc = m_java_widget->minHeapSize();
    conf.memory.maxAlloc = m_java_widget->maxHeapSize();
    if (m_java_widget->permGenEnabled()) {
        conf.memory.permGen = m_java_widget->permGenSize();
    } else {
        // FIXME: don't hardcode
        conf.memory.permGen = 128;
    }
    return true;
}

void JavaWizardPage::retranslate()
{
    setTitle(tr("Java"));
    setSubTitle(
        tr("Please select how much memory to allocate to instances and if Prism Launcher should manage Java automatically or manually."));
    m_java_widget->retranslate();
}
