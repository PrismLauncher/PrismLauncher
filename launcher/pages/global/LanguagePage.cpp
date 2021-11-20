#include "LanguagePage.h"

#include "widgets/LanguageSelectionWidget.h"
#include <QVBoxLayout>

LanguagePage::LanguagePage(QWidget* parent) :
    QWidget(parent)
{
    setObjectName(QStringLiteral("languagePage"));
    auto layout = new QVBoxLayout(this);
    mainWidget = new LanguageSelectionWidget(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(mainWidget);
    retranslate();
}

LanguagePage::~LanguagePage()
{
}

bool LanguagePage::apply()
{
    applySettings();
    return true;
}

void LanguagePage::applySettings()
{
    auto settings = APPLICATION->settings();
    QString key = mainWidget->getSelectedLanguageKey();
    settings->set("Language", key);
}

void LanguagePage::loadSettings()
{
    // NIL
}

void LanguagePage::retranslate()
{
    mainWidget->retranslate();
}

void LanguagePage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}
