// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "InstallJavaDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

#include "Application.h"
#include "BaseVersionList.h"
#include "FileSystem.h"
#include "Filter.h"
#include "java/download/ArchiveDownloadTask.h"
#include "java/download/ManifestDownloadTask.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/java/VersionList.h"
#include "ui/widgets/PageContainer.h"
#include "ui/widgets/VersionSelectWidget.h"

#if defined(Q_OS_MACOS)
#include "java/download/SymlinkTask.h"
#include "tasks/SequentialTask.h"
#endif

namespace {
class InstallJavaPage : public QWidget, public BasePage {
   public:
    Q_OBJECT
   public:
    explicit InstallJavaPage(QString id, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_uid(std::move(id))
        , m_horizontalLayout(new QHBoxLayout(this))
        , m_majorVersionSelect(new VersionSelectWidget(this))
        , m_javaVersionSelect(new VersionSelectWidget(this))
    {
        setObjectName(QStringLiteral("VersionSelectWidget"));

        m_horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        m_horizontalLayout->setContentsMargins(0, 0, 0, 0);

        m_majorVersionSelect->selectCurrent();
        m_majorVersionSelect->setEmptyString(tr("No Java versions are currently available in the meta."));
        m_majorVersionSelect->setEmptyErrorString(tr("Couldn't load or download the Java version lists!"));
        m_horizontalLayout->addWidget(m_majorVersionSelect, 1);

        m_javaVersionSelect->setEmptyString(tr("No Java versions are currently available for your OS."));
        m_javaVersionSelect->setEmptyErrorString(tr("Couldn't load or download the Java version lists!"));
        m_horizontalLayout->addWidget(m_javaVersionSelect, 4);
        connect(m_majorVersionSelect, &VersionSelectWidget::selectedVersionChanged, this, &InstallJavaPage::setSelectedVersion);
        connect(m_majorVersionSelect, &VersionSelectWidget::selectedVersionChanged, this, &InstallJavaPage::selectionChanged);
        connect(m_javaVersionSelect, &VersionSelectWidget::selectedVersionChanged, this, &InstallJavaPage::selectionChanged);

        QMetaObject::connectSlotsByName(this);
    }
    ~InstallJavaPage() override
    {
        delete m_horizontalLayout;
        delete m_majorVersionSelect;
        delete m_javaVersionSelect;
    }

    static PageInfo page(const QString& id, const QString& iconName, const char* displayName)
    {
        return {
            .id = id,
            .displayName = displayName,
            .icon = QIcon::fromTheme(iconName),
            .helpPage = "Loader-mods",
            .constructor = [id](QWidget* parent) { return new InstallJavaPage(id, parent); },
        };
    }

    //! loads the list if needed.
    void initialize(const Meta::VersionList::Ptr& vlist)
    {
        vlist->setProvidedRoles({ BaseVersionList::JavaMajorRole, BaseVersionList::RecommendedRole, BaseVersionList::VersionPointerRole });
        m_majorVersionSelect->initialize(vlist.get());
    }

    void setSelectedVersion(const BaseVersion::Ptr& version)
    {
        auto dcast = std::dynamic_pointer_cast<Meta::Version>(version);
        if (!dcast) {
            return;
        }
        m_javaVersionSelect->initialize(new Java::VersionList(dcast, this));
        m_javaVersionSelect->selectCurrent();
    }

    void openedImpl() override
    {
        if (m_loaded) {
            return;
        }

        const auto versions = APPLICATION->metadataIndex()->get(m_uid);
        if (!versions) {
            return;
        }

        initialize(versions);
        m_loaded = true;
    }

    void setParentContainer(BasePageContainer* container) override
    {
        auto* dialog = dynamic_cast<QDialog*>(dynamic_cast<PageContainer*>(container)->parent());
        connect(m_javaVersionSelect->view(), &QAbstractItemView::doubleClicked, dialog, &QDialog::accept);
    }

    BaseVersion::Ptr selectedVersion() const { return m_javaVersionSelect->selectedVersion(); }
    void selectSearch() { m_javaVersionSelect->selectSearch(); }
    void loadList()
    {
        m_majorVersionSelect->loadList(true);
        m_javaVersionSelect->loadList(true);
    }

   public slots:
    void setRecommendedMajors(const QStringList& majors)
    {
        m_recommendedMajors = majors;
        recommendedFilterChanged();
    }
    void setRecommend(bool recommend)
    {
        m_recommend = recommend;
        recommendedFilterChanged();
    }
    void recommendedFilterChanged()
    {
        if (m_recommend) {
            m_majorVersionSelect->setFilter(BaseVersionList::ModelRoles::JavaMajorRole, Filters::equalsAny(m_recommendedMajors));
        } else {
            m_majorVersionSelect->setFilter(BaseVersionList::ModelRoles::JavaMajorRole, Filters::equalsAny());
        }
    }

   signals:
    void selectionChanged();

   private:
    const QString m_uid;
    bool m_loaded = false;

    QHBoxLayout* m_horizontalLayout = nullptr;
    VersionSelectWidget* m_majorVersionSelect = nullptr;
    VersionSelectWidget* m_javaVersionSelect = nullptr;

    QStringList m_recommendedMajors;
    bool m_recommend{};
};

InstallJavaPage* pageCast(QWidget* page)
{
    auto* result = dynamic_cast<InstallJavaPage*>(page);
    Q_ASSERT(result != nullptr);
    return result;
}

QStringList getRecommendedJavaVersionsFromVersionList(const Meta::VersionList::Ptr& list)
{
    QStringList recommendedJavas;
    for (const auto& ver : list->versions()) {
        auto major = ver->version();
        if (major.startsWith("java")) {
            major = "Java " + major.mid(4);
        }
        recommendedJavas.append(major);
    }
    return recommendedJavas;
}
}  // namespace
namespace Java {

InstallDialog::InstallDialog(const QString& uid, MinecraftInstance* instance, QWidget* parent)
    : QDialog(parent), m_container(new PageContainer(this, QString(), this)), m_buttons(new QDialogButtonBox(this))
{
    // FIXME: we need this since the code heavily relies on iterating the pages
    m_container->forceLoadWidgets();

    auto* layout = new QVBoxLayout(this);
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    layout->setContentsMargins(0, 0, 0, 0);
#endif
    m_container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(m_container);

    auto* buttonLayout = new QHBoxLayout(this);
// small margins look ugly on macOS on modal windows
#ifndef Q_OS_MACOS
    buttonLayout->setContentsMargins(0, 0, 6, 6);
#endif

    auto* refreshLayout = new QHBoxLayout(this);

    auto* refreshButton = new QPushButton(tr("&Refresh"), this);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        if (m_container->selectedPageWidget() == nullptr) {
            return;
        }
        pageCast(m_container->selectedPageWidget())->loadList();
    });
    refreshLayout->addWidget(refreshButton);

    auto* recommendedCheckBox = new QCheckBox("Recommended", this);
    recommendedCheckBox->setCheckState(Qt::CheckState::Checked);
    connect(recommendedCheckBox, &QCheckBox::checkStateChanged, this, [this](int state) {
        for (QWidget* page : m_container->pageWidgets()) {
            pageCast(page)->setRecommend(state == Qt::Checked);
        }
    });

    refreshLayout->addWidget(recommendedCheckBox);
    buttonLayout->addLayout(refreshLayout);

    m_buttons->setOrientation(Qt::Horizontal);
    m_buttons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Download"));
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(m_buttons);

    m_container->addButtons(buttonLayout);

    setWindowTitle(dialogTitle());
    setWindowModality(Qt::WindowModal);
    resize(840, 480);

    QStringList recommendedJavas;
    if (instance != nullptr) {
        auto mc = instance->getPackProfile()->getComponent("net.minecraft");
        if (mc) {
            auto file = mc->getVersionFile();  // no need for load as it should already be loaded
            if (file) {
                for (auto major : file->compatibleJavaMajors) {
                    recommendedJavas.append(QString("Java %1").arg(major));
                }
            }
        }
    } else {
        const auto versions = APPLICATION->metadataIndex()->get("net.minecraft.java");
        if (versions) {
            if (versions->isLoaded()) {
                recommendedJavas = getRecommendedJavaVersionsFromVersionList(versions);
            } else {
                auto newTask = versions->getLoadTask();
                if (newTask) {
                    connect(newTask.get(), &Task::succeeded, this, [this, versions] {
                        auto recommendedJavas = getRecommendedJavaVersionsFromVersionList(versions);
                        for (QWidget* page : m_container->pageWidgets()) {
                            pageCast(page)->setRecommendedMajors(recommendedJavas);
                        }
                    });
                    if (!newTask->isRunning()) {
                        newTask->start();
                    }
                } else {
                    recommendedJavas = getRecommendedJavaVersionsFromVersionList(versions);
                }
            }
        }
    }
    m_container->selectPage(uid);
    for (QWidget* page : m_container->pageWidgets()) {
        auto* cast = pageCast(page);
        cast->setRecommend(true);
        connect(cast, &InstallJavaPage::selectionChanged, this, [this, cast] { validate(cast); });
        if (!recommendedJavas.isEmpty()) {
            cast->setRecommendedMajors(recommendedJavas);
        }
    }
    connect(m_container, &PageContainer::selectedPageChanged, this,
            [this](QWidget* /*previous*/, QWidget* selected) { validate(selected); });
    if (m_container->selectedPageWidget() != nullptr) {
        pageCast(m_container->selectedPageWidget())->selectSearch();
    }
    validate(m_container->selectedPageWidget());
}

QList<PageInfo> InstallDialog::getPages()
{
    return {
        // Mojang
        InstallJavaPage::page("net.minecraft.java", "mojang", "Mojang"),
        // Adoptium
        InstallJavaPage::page("net.adoptium.java", "adoptium", "Adoptium"),
        // Azul
        InstallJavaPage::page("com.azul.java", "azul", "Azul Zulu"),
        // IBM
        /* Must watch out in case the AdoptOpenJDK infrastructure is deprecated.
        In case of happening, IBM does not seem to provide as of today (03/2026) an API like Adoptium does and rather uses GitHub directly
        in its website: `developer.ibm.com`. GitHub is known for rate limiting requests that do not use an API key from an account. */
        InstallJavaPage::page("com.ibm.java", "openj9_hex_custom", "IBM Semeru Open"),
    };
}

QString InstallDialog::dialogTitle()
{
    return tr("Install Java");
}

void InstallDialog::validate(QWidget* selected)
{
    m_buttons->button(QDialogButtonBox::Ok)
        ->setEnabled(selected != nullptr && !!std::dynamic_pointer_cast<Java::Metadata>(pageCast(selected)->selectedVersion()));
}

void InstallDialog::done(int result)
{
    if (result == Accepted) {
        auto* page = pageCast(m_container->selectedPageWidget());
        if (page->selectedVersion()) {
            auto meta = std::dynamic_pointer_cast<Java::Metadata>(page->selectedVersion());
            if (meta) {
                Task::Ptr task;
                auto finalPath = FS::PathCombine(APPLICATION->javaPath(), meta->m_name);
                auto deletePath = [finalPath] { FS::deletePath(finalPath); };
                switch (meta->downloadType) {
                    case Java::DownloadType::Manifest:
                        task = makeShared<ManifestDownloadTask>(meta->url, finalPath, meta->checksumType, meta->checksumHash);
                        break;
                    case Java::DownloadType::Archive:
                        task = makeShared<ArchiveDownloadTask>(meta->url, finalPath, meta->checksumType, meta->checksumHash);
                        break;
                    case Java::DownloadType::Unknown:
                        QString error = QString(tr("Could not determine Java download type!"));
                        CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)->show();
                        deletePath();
                        return;
                }
#if defined(Q_OS_MACOS)
                auto seq = makeShared<SequentialTask>(tr("Install Java"));
                seq->addTask(task);
                seq->addTask(makeShared<Java::SymlinkTask>(finalPath));
                task = seq;
#endif
                connect(task.get(), &Task::failed, this, [this, &deletePath](const QString& reason) {
                    QString error = QString("Java download failed: %1").arg(reason);
                    CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)->show();
                    deletePath();
                });
                connect(task.get(), &Task::aborted, this, deletePath);
                ProgressDialog pg(this);
                pg.showSkipButton();
                pg.execWithTask(task.get());
            } else {
                return;
            }
        } else {
            return;
        }
    }

    QDialog::done(result);
}

}  // namespace Java

#include "InstallJavaDialog.moc"
