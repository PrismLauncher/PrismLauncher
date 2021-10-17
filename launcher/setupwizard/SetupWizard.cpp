#include "SetupWizard.h"

#include "LanguageWizardPage.h"
#include "JavaWizardPage.h"
#include "AnalyticsWizardPage.h"

#include "translations/TranslationsModel.h"
#include <Launcher.h>
#include <FileSystem.h>
#include <ganalytics.h>

#include <QAbstractButton>
#include <BuildConfig.h>

SetupWizard::SetupWizard(QWidget *parent) : QWizard(parent)
{
    setObjectName(QStringLiteral("SetupWizard"));
    resize(615, 659);
    // make it ugly everywhere to avoid variability in theming
    setWizardStyle(QWizard::ClassicStyle);
    setOptions(QWizard::NoCancelButton | QWizard::IndependentPages | QWizard::HaveCustomButton1);

    retranslate();

    connect(this, &QWizard::currentIdChanged, this, &SetupWizard::pageChanged);
}

void SetupWizard::retranslate()
{
    setButtonText(QWizard::NextButton, tr("&Next >"));
    setButtonText(QWizard::BackButton, tr("< &Back"));
    setButtonText(QWizard::FinishButton, tr("&Finish"));
    setButtonText(QWizard::CustomButton1, tr("&Refresh"));
    setWindowTitle(tr("%1 Quick Setup").arg(BuildConfig.LAUNCHER_NAME));
}

BaseWizardPage * SetupWizard::getBasePage(int id)
{
    if(id == -1)
        return nullptr;
    auto pagePtr = page(id);
    if(!pagePtr)
        return nullptr;
    return dynamic_cast<BaseWizardPage *>(pagePtr);
}

BaseWizardPage * SetupWizard::getCurrentBasePage()
{
    return getBasePage(currentId());
}

void SetupWizard::pageChanged(int id)
{
    auto basePagePtr = getBasePage(id);
    if(!basePagePtr)
    {
        return;
    }
    if(basePagePtr->wantsRefreshButton())
    {
        setButtonLayout({QWizard::CustomButton1, QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton});
        auto customButton = button(QWizard::CustomButton1);
        connect(customButton, &QAbstractButton::pressed, [&](){
            auto basePagePtr = getCurrentBasePage();
            if(basePagePtr)
            {
                basePagePtr->refresh();
            }
        });
    }
    else
    {
        setButtonLayout({QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton});
    }
}


void SetupWizard::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWizard::changeEvent(event);
}

SetupWizard::~SetupWizard()
{
}
