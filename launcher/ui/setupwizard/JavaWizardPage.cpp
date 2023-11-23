#include "JavaWizardPage.h"
#include "Application.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <sys.h>

#include "FileSystem.h"
#include "JavaCommon.h"
#include "java/JavaInstall.h"
#include "java/JavaUtils.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/widgets/JavaSettingsWidget.h"
#include "ui/widgets/VersionSelectWidget.h"

JavaWizardPage::JavaWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    setupUi();
}

void JavaWizardPage::setupUi()
{
    setObjectName(QStringLiteral("javaPage"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_java_widget = new JavaSettingsWidget(this);
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
    auto settings = APPLICATION->settings();
    auto result = m_java_widget->validate();
    switch (result) {
        default:
        case JavaSettingsWidget::ValidationStatus::Bad: {
            return false;
        }
        case JavaSettingsWidget::ValidationStatus::AllOK: {
            settings->set("JavaPath", m_java_widget->javaPath());
        } /* fallthrough */
        case JavaSettingsWidget::ValidationStatus::JavaBad: {
            // Memory
            auto s = APPLICATION->settings();
            s->set("MinMemAlloc", m_java_widget->minHeapSize());
            s->set("MaxMemAlloc", m_java_widget->maxHeapSize());
            if (m_java_widget->permGenEnabled()) {
                s->set("PermGen", m_java_widget->permGenSize());
            } else {
                s->reset("PermGen");
            }
            return true;
        }
    }
}

void JavaWizardPage::retranslate()
{
    setTitle(tr("Java"));
    setSubTitle(
        tr("You do not have a working Java set up yet or it went missing.\n"
           "Please select one of the following or browse for a Java executable."));
    m_java_widget->retranslate();
}
