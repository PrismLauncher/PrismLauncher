#include "ResourcePage.h"
#include "ui_ResourcePage.h"

#include <QDesktopServices>
#include <QKeyEvent>

#include "Markdown.h"
#include "ResourceDownloadTask.h"

#include "minecraft/MinecraftInstance.h"

#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/pages/modplatform/ResourceModel.h"
#include "ui/widgets/ProjectItem.h"

ResourcePage::ResourcePage(ResourceDownloadDialog* parent, BaseInstance& base_instance)
    : QWidget(parent), m_base_instance(base_instance), m_ui(new Ui::ResourcePage), m_parent_dialog(parent), m_fetch_progress(this, false)
{
    m_ui->setupUi(this);

    m_ui->searchEdit->installEventFilter(this);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_search_timer.setTimerType(Qt::TimerType::CoarseTimer);
    m_search_timer.setSingleShot(true);

    connect(&m_search_timer, &QTimer::timeout, this, &ResourcePage::triggerSearch);

    m_fetch_progress.hideIfInactive(true);
    m_fetch_progress.setFixedHeight(24);
    m_fetch_progress.progressFormat("");

    m_ui->gridLayout_3->addWidget(&m_fetch_progress, 0, 0, 1, m_ui->gridLayout_3->columnCount());

    m_ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    m_ui->packView->installEventFilter(this);

    connect(m_ui->packDescription, &QTextBrowser::anchorClicked, this, &ResourcePage::openUrl);
}

ResourcePage::~ResourcePage()
{
    delete m_ui;
}

void ResourcePage::retranslate()
{
    m_ui->retranslateUi(this);
}

void ResourcePage::openedImpl()
{
    if (!supportsFiltering())
        m_ui->resourceFilterButton->setVisible(false);

    updateSelectionButton();
    triggerSearch();
}

auto ResourcePage::eventFilter(QObject* watched, QEvent* event) -> bool
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (watched == m_ui->searchEdit) {
            if (keyEvent->key() == Qt::Key_Return) {
                triggerSearch();
                keyEvent->accept();
                return true;
            } else {
                if (m_search_timer.isActive())
                    m_search_timer.stop();

                m_search_timer.start(350);
            }
        } else if (watched == m_ui->packView) {
            if (keyEvent->key() == Qt::Key_Return) {
                onResourceSelected();

                // To have the 'select mod' button outlined instead of the 'review and confirm' one
                m_ui->resourceSelectionButton->setFocus(Qt::FocusReason::ShortcutFocusReason);
                m_ui->packView->setFocus(Qt::FocusReason::NoFocusReason);

                keyEvent->accept();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

QString ResourcePage::getSearchTerm() const
{
    return m_ui->searchEdit->text();
}

void ResourcePage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}

ModPlatform::IndexedPack ResourcePage::getCurrentPack() const
{
    return m_model->data(m_ui->packView->currentIndex(), Qt::UserRole).value<ModPlatform::IndexedPack>();
}

bool ResourcePage::isPackSelected(const ModPlatform::IndexedPack& pack, int version) const
{
    if (version < 0 || !pack.versionsLoaded)
        return m_parent_dialog->isSelected(pack.name);

    return m_parent_dialog->isSelected(pack.name, pack.versions[version].fileName);
}

void ResourcePage::updateUi()
{
    auto current_pack = getCurrentPack();

    QString text = "";
    QString name = current_pack.name;

    if (current_pack.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current_pack.websiteUrl + "\">" + name + "</a>";

    if (!current_pack.authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) -> QString {
            if (author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : current_pack.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }

    if (current_pack.extraDataLoaded) {
        if (!current_pack.extraData.donate.isEmpty()) {
            text += "<br><br>" + tr("Donate information: ");
            auto donateToStr = [](ModPlatform::DonationData& donate) -> QString {
                return QString("<a href=\"%1\">%2</a>").arg(donate.url, donate.platform);
            };
            QStringList donates;
            for (auto& donate : current_pack.extraData.donate) {
                donates.append(donateToStr(donate));
            }
            text += donates.join(", ");
        }

        if (!current_pack.extraData.issuesUrl.isEmpty() || !current_pack.extraData.sourceUrl.isEmpty() ||
            !current_pack.extraData.wikiUrl.isEmpty() || !current_pack.extraData.discordUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!current_pack.extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(current_pack.extraData.issuesUrl) + "<br>";
        if (!current_pack.extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(current_pack.extraData.wikiUrl) + "<br>";
        if (!current_pack.extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(current_pack.extraData.sourceUrl) + "<br>";
        if (!current_pack.extraData.discordUrl.isEmpty())
            text += "- " + tr("Discord: <a href=%1>%1</a>").arg(current_pack.extraData.discordUrl) + "<br>";
    }

    text += "<hr>";

    m_ui->packDescription->setHtml(
        text + (current_pack.extraData.body.isEmpty() ? current_pack.description : markdownToHTML(current_pack.extraData.body)));
    m_ui->packDescription->flush();
}

void ResourcePage::updateSelectionButton()
{
    if (!isOpened || m_selected_version_index < 0) {
        m_ui->resourceSelectionButton->setEnabled(false);
        return;
    }

    m_ui->resourceSelectionButton->setEnabled(true);
    if (!isPackSelected(getCurrentPack(), m_selected_version_index)) {
        m_ui->resourceSelectionButton->setText(tr("Select %1 for download").arg(resourceString()));
    } else {
        m_ui->resourceSelectionButton->setText(tr("Deselect %1 for download").arg(resourceString()));
    }
}

void ResourcePage::updateVersionList()
{
    auto current_pack = getCurrentPack();

    m_ui->versionSelectionBox->blockSignals(true);
    m_ui->versionSelectionBox->clear();
    m_ui->versionSelectionBox->blockSignals(false);

    for (int i = 0; i < current_pack.versions.size(); i++) {
        auto& version = current_pack.versions[i];
        if (optedOut(version))
            continue;

        m_ui->versionSelectionBox->addItem(current_pack.versions[i].version, QVariant(i));
    }

    if (m_ui->versionSelectionBox->count() == 0) {
        m_ui->versionSelectionBox->addItem(tr("No valid version found."), QVariant(-1));
        m_ui->resourceSelectionButton->setText(tr("Cannot select invalid version :("));
    }

    updateSelectionButton();
}

void ResourcePage::onSelectionChanged(QModelIndex curr, QModelIndex prev)
{
    if (!curr.isValid()) {
        return;
    }

    auto current_pack = getCurrentPack();

    bool request_load = false;
    if (!current_pack.versionsLoaded) {
        m_ui->resourceSelectionButton->setText(tr("Loading versions..."));
        m_ui->resourceSelectionButton->setEnabled(false);

        request_load = true;
    } else {
        updateVersionList();
    }

    if (!current_pack.extraDataLoaded)
        request_load = true;

    if (request_load)
        m_model->loadEntry(curr);

    updateUi();
}

void ResourcePage::onVersionSelectionChanged(QString data)
{
    if (data.isNull() || data.isEmpty()) {
        m_selected_version_index = -1;
        return;
    }

    m_selected_version_index = m_ui->versionSelectionBox->currentData().toInt();
    updateSelectionButton();
}

void ResourcePage::addResourceToDialog(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& version)
{
    m_parent_dialog->addResource(pack.name, new ResourceDownloadTask(pack, version, m_parent_dialog->getBaseModel()));
}

void ResourcePage::removeResourceFromDialog(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion&)
{
    m_parent_dialog->removeResource(pack.name);
}

void ResourcePage::onResourceSelected()
{
    if (m_selected_version_index < 0)
        return;

    auto current_pack = getCurrentPack();

    auto& version = current_pack.versions[m_selected_version_index];
    if (m_parent_dialog->isSelected(current_pack.name, version.fileName))
        removeResourceFromDialog(current_pack, version);
    else
        addResourceToDialog(current_pack, version);

    updateSelectionButton();

    /* Force redraw on the resource list when the selection changes */
    m_ui->packView->adjustSize();
}

void ResourcePage::openUrl(const QUrl& url)
{
    // do not allow other url schemes for security reasons
    if (!(url.scheme() == "http" || url.scheme() == "https")) {
        qWarning() << "Unsupported scheme" << url.scheme();
        return;
    }

    // detect URLs and search instead

    const QString address = url.host() + url.path();
    QRegularExpressionMatch match;
    QString page;

    for (auto&& [regex, candidate] : urlHandlers().asKeyValueRange()) {
        if (match = QRegularExpression(regex).match(address); match.hasMatch()) {
            page = candidate;
            break;
        }
    }

    if (!page.isNull()) {
        const QString slug = match.captured(1);

        // ensure the user isn't opening the same mod
        if (slug != getCurrentPack().slug) {
            m_parent_dialog->selectPage(page);

            auto newPage = m_parent_dialog->getSelectedPage();

            QLineEdit* searchEdit = newPage->m_ui->searchEdit;
            auto model = newPage->m_model;
            QListView* view = newPage->m_ui->packView;

            auto jump = [url, slug, model, view] {
                for (int row = 0; row < model->rowCount({}); row++) {
                    const QModelIndex index = model->index(row);
                    const auto pack = model->data(index, Qt::UserRole).value<ModPlatform::IndexedPack>();

                    if (pack.slug == slug) {
                        view->setCurrentIndex(index);
                        return;
                    }
                }

                // The final fallback.
                QDesktopServices::openUrl(url);
            };

            searchEdit->setText(slug);
            newPage->triggerSearch();

            if (model->activeJob().isRunning())
                connect(&model->activeJob(), &Task::finished, jump);
            else
                jump();

            return;
        }
    }

    // open in the user's web browser
    QDesktopServices::openUrl(url);
}
