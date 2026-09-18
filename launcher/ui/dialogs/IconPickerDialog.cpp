/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <utility>

#include <QFileDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>

#include "Application.h"

#include "IconPickerDialog.h"
#include "ui_IconPickerDialog.h"

#include "ui/instanceview/InstanceDelegate.h"

#include <DesktopServices.h>
#include "icons/IconList.h"
#include "icons/IconUtils.h"

namespace {

class IconProxyModel : public QSortFilterProxyModel {
   public:
    explicit IconProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setCategory(IconPickerDialog::IconPickerCategory category)
    {
        if (m_category == category) {
            return;
        }
        m_category = category;
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
        invalidateFilter();
#else
        beginFilterChange();
        endFilterChange();
#endif
    }

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
            return false;
        }

        if (m_category == IconPickerDialog::IconPickerCategory::Any) {
            return true;
        }

        auto* model = static_cast<IconList*>(sourceModel());
        const QModelIndex index = model->index(sourceRow, 0, sourceParent);
        QString key = model->data(index, Qt::UserRole).toString();
        const MMCIcon* icon = model->icon(key);

        if (!icon) {
            return false;
        }

        bool isModpack = false;
        bool isBuiltin = icon->isBuiltIn();
        bool isLegacy = isBuiltin && icon->name().endsWith("_legacy", Qt::CaseInsensitive);

        if (!isBuiltin) {
            const QString& name = icon->name();
            if (name.startsWith("curseforge_", Qt::CaseInsensitive) || name.startsWith("modrinth_", Qt::CaseInsensitive) ||
                name.startsWith("ftb_", Qt::CaseInsensitive) || name.startsWith("technic_", Qt::CaseInsensitive) ||
                name.startsWith("atl_", Qt::CaseInsensitive)) {
                isModpack = true;
            }
        }

        switch (m_category) {
            case IconPickerDialog::IconPickerCategory::Legacy:
                return isBuiltin && isLegacy;
            case IconPickerDialog::IconPickerCategory::Modpacks:
                return isModpack;
            case IconPickerDialog::IconPickerCategory::Modern:
                return isBuiltin && !isLegacy;
            case IconPickerDialog::IconPickerCategory::Custom:
                return !isBuiltin && !isModpack;
            default:
                return true;
        }
    }

   private:
    IconPickerDialog::IconPickerCategory m_category = IconPickerDialog::IconPickerCategory::Any;
};
}  // namespace

IconPickerDialog::IconPickerDialog(QWidget* parent) : QDialog(parent), m_ui(new Ui::IconPickerDialog)
{
    m_ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    static const std::array s_context_text = {
        tr("All"), tr("Modern"), tr("Legacy"), tr("Modpacks"), tr("Custom"),
    };
    static const std::array s_context_id = {
        IconPickerCategory::Any,      IconPickerCategory::Modern, IconPickerCategory::Legacy,
        IconPickerCategory::Modpacks, IconPickerCategory::Custom,
    };
    for (int i = 0; std::cmp_less(i, s_context_text.size()); ++i) {
        m_ui->contextCombo->addItem(s_context_text.at(i), static_cast<int>(s_context_id.at(i)));
        if (i == 0) {
            m_ui->contextCombo->insertSeparator(i + 1);
        }
    }

    m_proxyModel = new IconProxyModel(this);
    m_proxyModel->setSourceModel(APPLICATION->icons());
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_ui->iconView->setModel(m_proxyModel);

    auto* contentsWidget = m_ui->iconView;
    contentsWidget->setFlow(QListView::LeftToRight);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    contentsWidget->setSpacing(5);
    contentsWidget->setWordWrap(false);
    contentsWidget->setWrapping(true);
    contentsWidget->setUniformItemSizes(true);
    contentsWidget->setTextElideMode(Qt::ElideRight);
    contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setItemDelegate(new ListViewDelegate(contentsWidget));

    // contentsWidget->setAcceptDrops(true);
    contentsWidget->setDropIndicatorShown(true);
    contentsWidget->viewport()->setAcceptDrops(true);
    contentsWidget->setDragDropMode(QAbstractItemView::DropOnly);
    contentsWidget->setDefaultDropAction(Qt::CopyAction);

    contentsWidget->installEventFilter(this);

    contentsWidget->setModel(m_proxyModel);

    // NOTE: ResetRole forces the button to be on the left, while the OK/Cancel ones are on the right. We win.
    auto* buttonAdd = m_ui->buttonBox->addButton(tr("Add Icon"), QDialogButtonBox::ResetRole);
    m_buttonRemove = m_ui->buttonBox->addButton(tr("Remove Icon"), QDialogButtonBox::ResetRole);

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonAdd, &QPushButton::clicked, this, &IconPickerDialog::addNewIcon);
    connect(m_buttonRemove, &QPushButton::clicked, this, &IconPickerDialog::removeSelectedIcon);

    connect(contentsWidget, &QAbstractItemView::doubleClicked, this, &IconPickerDialog::activated);

    connect(contentsWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &IconPickerDialog::selectionChanged);

    auto* buttonFolder = m_ui->buttonBox->addButton(tr("Open Folder"), QDialogButtonBox::ResetRole);
    connect(buttonFolder, &QPushButton::clicked, this, &IconPickerDialog::openFolder);
    connect(m_ui->searchLine, &QLineEdit::textChanged, this, &IconPickerDialog::filterIcons);
    connect(m_ui->contextCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const IconPickerCategory category = static_cast<IconPickerCategory>(m_ui->contextCombo->itemData(index).toInt());
        filterIconsByCategory(category);
    });
    // Prevent incorrect indices from e.g. filesystem changes
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this]() { m_proxyModel->invalidate(); });
}

bool IconPickerDialog::eventFilter(QObject* obj, QEvent* evt)
{
    if (obj != m_ui->iconView) {
        return QDialog::eventFilter(obj, evt);
    }
    if (evt->type() != QEvent::KeyPress) {
        return QDialog::eventFilter(obj, evt);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(evt);
    switch (keyEvent->key()) {
        case Qt::Key_Delete:
            removeSelectedIcon();
            return true;
        case Qt::Key_Plus:
            addNewIcon();
            return true;
        default:
            break;
    }
    return QDialog::eventFilter(obj, evt);
}

void IconPickerDialog::addNewIcon()
{
    //: The title of the select icons open file dialog
    QString selectIcons = tr("Select Icons");
    //: The type of icon files
    auto filter = IconUtils::getIconFilter();
    QStringList fileNames = QFileDialog::getOpenFileNames(this, selectIcons, QString(), tr("Icons %1").arg(filter));
    APPLICATION->icons()->installIcons(fileNames);
}

void IconPickerDialog::removeSelectedIcon() const
{
    if (APPLICATION->icons()->trashIcon(selectedIconKey)) {
        return;
    }

    APPLICATION->icons()->deleteIcon(selectedIconKey);
}

void IconPickerDialog::activated(QModelIndex index)
{
    selectedIconKey = index.data(Qt::UserRole).toString();
    accept();
}

void IconPickerDialog::selectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.empty()) {
        return;
    }

    QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
    if (!key.isEmpty()) {
        selectedIconKey = key;
    }
    m_buttonRemove->setEnabled(APPLICATION->icons()->iconFileExists(selectedIconKey));
}

int IconPickerDialog::execWithSelection(const QString& selection)
{
    auto* list = APPLICATION->icons();
    auto* contentsWidget = m_ui->iconView;
    selectedIconKey = selection;

    const int indexNr = list->getIconIndex(selection);
    auto modelIndex = list->index(indexNr);
    contentsWidget->selectionModel()->select(modelIndex, QItemSelectionModel::Current | QItemSelectionModel::Select);

    QMetaObject::invokeMethod(this, "delayed_scroll", Qt::QueuedConnection, Q_ARG(QModelIndex, modelIndex));
    return QDialog::exec();
}

void IconPickerDialog::delayed_scroll(QModelIndex modelIndex)
{
    auto* contentsWidget = m_ui->iconView;
    contentsWidget->scrollTo(modelIndex);
}

IconPickerDialog::~IconPickerDialog()
{
    delete m_ui;
}

void IconPickerDialog::openFolder() const
{
    DesktopServices::openPath(APPLICATION->icons()->iconDirectory(selectedIconKey), true);
}

void IconPickerDialog::filterIcons(const QString& text)
{
    m_proxyModel->setFilterFixedString(text);
}

void IconPickerDialog::filterIconsByCategory(IconPickerCategory category)
{
    static_cast<IconProxyModel*>(m_proxyModel)->setCategory(category);
}
