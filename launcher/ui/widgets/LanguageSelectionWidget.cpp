#include "LanguageSelectionWidget.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QTreeView>
#include <QVBoxLayout>
#include "Application.h"
#include "BuildConfig.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"
#include "translations/TranslationsModel.h"

LanguageSelectionWidget::LanguageSelectionWidget(QWidget* parent)
    : QWidget(parent)
    , m_verticalLayout(new QVBoxLayout(this))
    , m_languageView(new QTreeView(this))
    , m_helpUsLabel(new QLabel(this))
    , m_formatCheckbox(new QCheckBox(this))
{
    m_verticalLayout->setObjectName(QStringLiteral("verticalLayout"));

    m_helpUsLabel->setObjectName(QStringLiteral("helpUsLabel"));
    m_helpUsLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_helpUsLabel->setOpenExternalLinks(true);
    m_helpUsLabel->setWordWrap(true);
    m_verticalLayout->addWidget(m_helpUsLabel);

    m_formatCheckbox->setObjectName(QStringLiteral("formatCheckbox"));
    m_formatCheckbox->setCheckState(APPLICATION->settings()->get("UseSystemLocale").toBool() ? Qt::Checked : Qt::Unchecked);
    connect(m_formatCheckbox, &QCheckBox::checkStateChanged, this,
            [this] { APPLICATION->translations()->setUseSystemLocale(m_formatCheckbox->isChecked()); });
    m_verticalLayout->addWidget(m_formatCheckbox);

    auto* translations = APPLICATION->translations();
    auto index = translations->selectedIndex();
    m_languageView->setModel(translations);
    m_languageView->setCurrentIndex(index);
    m_languageView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_languageView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(m_languageView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LanguageSelectionWidget::languageRowChanged);
    m_verticalLayout->setContentsMargins(0, 0, 0, 0);

    auto languageSetting = APPLICATION->settings()->getSetting("Language");
    connect(languageSetting.get(), &Setting::SettingChanged, this, &LanguageSelectionWidget::languageSettingChanged);
}

QString LanguageSelectionWidget::getSelectedLanguageKey() const
{
    auto* translations = APPLICATION->translations();
    return translations->data(m_languageView->currentIndex(), Qt::UserRole).toString();
}

void LanguageSelectionWidget::retranslate()
{
    QString text = tr("Don't see your language or the quality is poor?<br/><a href=\"%1\">Help us with translations!</a>")
                       .arg(BuildConfig.TRANSLATIONS_URL);
    m_helpUsLabel->setText(text);
    m_formatCheckbox->setText(tr("Use system regional standards"));
}

void LanguageSelectionWidget::languageRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    if (current == previous) {
        return;
    }
    auto* translations = APPLICATION->translations();
    QString key = translations->data(current, Qt::UserRole).toString();
    translations->selectLanguage(key);
    translations->updateLanguage(key);
}

void LanguageSelectionWidget::languageSettingChanged(const Setting& /*unused*/, const QVariant& /*unused*/)
{
    auto* translations = APPLICATION->translations();
    auto index = translations->selectedIndex();
    m_languageView->setCurrentIndex(index);
}
