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

class IconProxyModel : public QSortFilterProxyModel
{
public:
    explicit IconProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent)
    {
    }

    void setCategory(IconPickerDialog::IconPickerCategory category)
    {
        if (m_category == category)
            return;
        m_category = category;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent))
            return false;

        if (m_category == IconPickerDialog::Any)
            return true;

        auto model = static_cast<IconList*>(sourceModel());
        QModelIndex index = model->index(source_row, 0, source_parent);
        QString key = model->data(index, Qt::UserRole).toString();
        const MMCIcon* icon = model->icon(key);

        if (!icon)
            return false;

        bool isModpack = false;
        bool isBuiltin = icon->isBuiltIn();
        bool isLegacy = isBuiltin && icon->name().endsWith("_legacy", Qt::CaseInsensitive);

        if (!isBuiltin) {
            const QString& name = icon->name();
            if (name.startsWith("curseforge_", Qt::CaseInsensitive) ||
                name.startsWith("modrinth_", Qt::CaseInsensitive) ||
                name.startsWith("ftb_", Qt::CaseInsensitive) ||
                name.startsWith("technic_", Qt::CaseInsensitive) ||
                name.startsWith("atl_", Qt::CaseInsensitive)) {
                isModpack = true;
            }
        }

        switch (m_category) {
            case IconPickerDialog::Legacy:
                return isBuiltin && isLegacy;
            case IconPickerDialog::Modpacks:
                return isModpack;
            case IconPickerDialog::Modern:
                return isBuiltin && !isLegacy;
            case IconPickerDialog::Custom:
                return !isBuiltin && !isModpack;
            default:
                return true;
        }
    }

private:
    IconPickerDialog::IconPickerCategory m_category = IconPickerDialog::Any;
};

IconPickerDialog::IconPickerDialog(QWidget* parent) : QDialog(parent), ui(new Ui::IconPickerDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    static const QString context_text[] = {
        tr("All"),
        tr("Modern"),
        tr("Legacy"),
        tr("Modpacks"),
        tr("Custom"),
    };
    static const IconPickerCategory context_id[] = {
        Any,
        Modern,
        Legacy,
        Modpacks,
        Custom,
    };
    const int cnt = sizeof(context_text) / sizeof(context_text[0]);
    for (int i = 0; i < cnt; ++i) {
        ui->contextCombo->addItem(context_text[i], context_id[i]);
        if (i == 0) {
            ui->contextCombo->insertSeparator(i + 1);
        }
    }

    proxyModel = new IconProxyModel(this);
    proxyModel->setSourceModel(APPLICATION->icons());
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->iconView->setModel(proxyModel);

    auto contentsWidget = ui->iconView;
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

    contentsWidget->setModel(proxyModel);

    // NOTE: ResetRole forces the button to be on the left, while the OK/Cancel ones are on the right. We win.
    auto buttonAdd = ui->buttonBox->addButton(tr("Add Icon"), QDialogButtonBox::ResetRole);
    buttonRemove = ui->buttonBox->addButton(tr("Remove Icon"), QDialogButtonBox::ResetRole);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonAdd, &QPushButton::clicked, this, &IconPickerDialog::addNewIcon);
    connect(buttonRemove, &QPushButton::clicked, this, &IconPickerDialog::removeSelectedIcon);

    connect(contentsWidget, &QAbstractItemView::doubleClicked, this, &IconPickerDialog::activated);

    connect(contentsWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &IconPickerDialog::selectionChanged);

    auto buttonFolder = ui->buttonBox->addButton(tr("Open Folder"), QDialogButtonBox::ResetRole);
    connect(buttonFolder, &QPushButton::clicked, this, &IconPickerDialog::openFolder);
    connect(ui->searchLine, &QLineEdit::textChanged, this, &IconPickerDialog::filterIcons);
    connect(ui->contextCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        IconPickerCategory category = static_cast<IconPickerCategory>(ui->contextCombo->itemData(index).toInt());
        filterIconsByCategory(category);
    });
    // Prevent incorrect indices from e.g. filesystem changes
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this]() { proxyModel->invalidate(); });
}

bool IconPickerDialog::eventFilter(QObject* obj, QEvent* evt)
{
    if (obj != ui->iconView)
        return QDialog::eventFilter(obj, evt);
    if (evt->type() != QEvent::KeyPress) {
        return QDialog::eventFilter(obj, evt);
    }
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
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

void IconPickerDialog::removeSelectedIcon()
{
    if (APPLICATION->icons()->trashIcon(selectedIconKey))
        return;

    APPLICATION->icons()->deleteIcon(selectedIconKey);
}

void IconPickerDialog::activated(QModelIndex index)
{
    selectedIconKey = index.data(Qt::UserRole).toString();
    accept();
}

void IconPickerDialog::selectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if (selected.empty())
        return;

    QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
    if (!key.isEmpty()) {
        selectedIconKey = key;
    }
    buttonRemove->setEnabled(APPLICATION->icons()->iconFileExists(selectedIconKey));
}

int IconPickerDialog::execWithSelection(QString selection)
{
    auto list = APPLICATION->icons();
    auto contentsWidget = ui->iconView;
    selectedIconKey = selection;

    int index_nr = list->getIconIndex(selection);
    auto model_index = list->index(index_nr);
    contentsWidget->selectionModel()->select(model_index, QItemSelectionModel::Current | QItemSelectionModel::Select);

    QMetaObject::invokeMethod(this, "delayed_scroll", Qt::QueuedConnection, Q_ARG(QModelIndex, model_index));
    return QDialog::exec();
}

void IconPickerDialog::delayed_scroll(QModelIndex model_index)
{
    auto contentsWidget = ui->iconView;
    contentsWidget->scrollTo(model_index);
}

IconPickerDialog::~IconPickerDialog()
{
    delete ui;
}

void IconPickerDialog::openFolder()
{
    DesktopServices::openPath(APPLICATION->icons()->iconDirectory(selectedIconKey), true);
}

void IconPickerDialog::filterIcons(const QString& query)
{
    proxyModel->setFilterFixedString(query);
}

void IconPickerDialog::filterIconsByCategory(IconPickerCategory category)
{
    static_cast<IconProxyModel*>(proxyModel)->setCategory(category);
}
