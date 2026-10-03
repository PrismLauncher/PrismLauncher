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

#include "ModListView.h"
#include <QDrag>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>

ModListView::ModListView(QWidget* parent) : QTreeView(parent)
{
    setAllColumnsShowFocus(true);
    setExpandsOnDoubleClick(false);
    setRootIsDecorated(false);
    setSortingEnabled(true);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setHeaderHidden(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setDropIndicatorShown(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    viewport()->setAcceptDrops(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(header(), &QHeaderView::sectionResized, this, &ModListView::onSectionResized);
}

void ModListView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);
    auto head = header();
    head->setStretchLastSection(false);
    // HACK: this is true for the checkbox column of mod lists
    auto string = model->headerData(0, head->orientation()).toString();
    if (head->count() < 1) {
        return;
    }
    m_principalColumn = string.size() ? 0 : 1;
    for (int i = 0; i < head->count(); i++)
        head->setSectionResizeMode(i, QHeaderView::Interactive);
}

void ModListView::setResizeModes(const QList<QHeaderView::ResizeMode>& modes)
{
    auto head = header();
    for (int i = 0; i < modes.count(); i++) {
        // Stretch marks the principal column. Qt does not let the user drag a Stretch section,
        // so keep it Interactive and fill the spare width by hand.
        if (modes[i] == QHeaderView::Stretch) {
            m_principalColumn = i;
            head->setSectionResizeMode(i, QHeaderView::Interactive);
        } else {
            head->setSectionResizeMode(i, modes[i]);
        }
    }
}

void ModListView::resizeEvent(QResizeEvent* event)
{
    QTreeView::resizeEvent(event);
    giveSpareWidthToPrincipalColumn();
}

// Makes the principal column take whatever width the other visible columns leave.
void ModListView::giveSpareWidthToPrincipalColumn()
{
    auto head = header();
    if (m_principalColumn < 0 || m_principalColumn >= head->count() || m_adjustingColumnSizes)
        return;

    int others = 0;
    for (int i = 0; i < head->count(); i++) {
        if (i != m_principalColumn && !head->isSectionHidden(i))
            others += head->sectionSize(i);
    }

    m_adjustingColumnSizes = true;
    head->resizeSection(m_principalColumn, qMax(head->minimumSectionSize(), viewport()->width() - others));
    m_adjustingColumnSizes = false;
}

// Dragging a handle moves width between the two columns next to it, so the handle follows the cursor
// and the table stays filled. The right edge of the last column trades width with the principal column.
void ModListView::onSectionResized(int logicalIndex, int oldSize, int newSize)
{
    auto head = header();
    if (m_adjustingColumnSizes || m_principalColumn < 0)
        return;

    // A column was shown or hidden
    if (oldSize == 0 || newSize == 0) {
        giveSpareWidthToPrincipalColumn();
        return;
    }

    int neighbour = -1;
    for (int v = head->visualIndex(logicalIndex) + 1; v < head->count(); v++) {
        int i = head->logicalIndex(v);
        if (!head->isSectionHidden(i)) {
            neighbour = i;
            break;
        }
    }
    if (neighbour < 0)
        neighbour = m_principalColumn;
    if (neighbour == logicalIndex)
        return;

    int neighbourSize = head->sectionSize(neighbour) - (newSize - oldSize);
    int clamped = qMax(head->minimumSectionSize(), neighbourSize);

    m_adjustingColumnSizes = true;
    head->resizeSection(neighbour, clamped);
    // The neighbour hit its minimum width, so give the difference back
    if (clamped != neighbourSize)
        head->resizeSection(logicalIndex, newSize - (clamped - neighbourSize));
    m_adjustingColumnSizes = false;
}
