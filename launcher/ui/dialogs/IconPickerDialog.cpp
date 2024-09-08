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
#include <QPushButton>

#include "Application.h"

#include "IconPickerDialog.h"
#include "ui_IconPickerDialog.h"

#include "ui/instanceview/InstanceDelegate.h"

#include <DesktopServices.h>
#include "icons/IconList.h"
#include "icons/IconUtils.h"

IconPickerDialog::IconPickerDialog(QWidget* parent) : QDialog(parent), ui(new Ui::IconPickerDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    auto contentsWidget = ui->iconView;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setFlow(QListView::LeftToRight);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setResizeMode(QListView::Adjust);
    contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    contentsWidget->setSpacing(5);
    contentsWidget->setWordWrap(false);
    contentsWidget->setWrapping(true);
    contentsWidget->setUniformItemSizes(true);
    contentsWidget->setTextElideMode(Qt::ElideRight);
    contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setItemDelegate(new ListViewDelegate());

    // contentsWidget->setAcceptDrops(true);
    contentsWidget->setDropIndicatorShown(true);
    contentsWidget->viewport()->setAcceptDrops(true);
    contentsWidget->setDragDropMode(QAbstractItemView::DropOnly);
    contentsWidget->setDefaultDropAction(Qt::CopyAction);

    contentsWidget->installEventFilter(this);

    contentsWidget->setModel(APPLICATION->icons().get());

    // NOTE: ResetRole forces the button to be on the left, while the OK/Cancel ones are on the right. We win.
    auto buttonAdd = ui->buttonBox->addButton(tr("Add Icon"), QDialogButtonBox::ResetRole);
    buttonRemove = ui->buttonBox->addButton(tr("Remove Icon"), QDialogButtonBox::ResetRole);

    connect(buttonAdd, SIGNAL(clicked(bool)), SLOT(addNewIcon()));
    connect(buttonRemove, SIGNAL(clicked(bool)), SLOT(removeSelectedIcon()));

    connect(contentsWidget, SIGNAL(doubleClicked(QModelIndex)), SLOT(activated(QModelIndex)));

    connect(contentsWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            SLOT(selectionChanged(QItemSelection, QItemSelection)));

    auto buttonFolder = ui->buttonBox->addButton(tr("Open Folder"), QDialogButtonBox::ResetRole);
    connect(buttonFolder, &QPushButton::clicked, this, &IconPickerDialog::openFolder);
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
    DesktopServices::openPath(APPLICATION->icons()->getDirectory(), true);
}
