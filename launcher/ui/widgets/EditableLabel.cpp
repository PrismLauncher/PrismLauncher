// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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

#include "EditableLabel.h"

#include <QBoxLayout>
#include <QEvent>
#include <QFrame>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtAssert>

namespace {
void autoResize(QTextEdit* edit)
{
    edit->setFixedHeight(static_cast<int>(edit->document()->size().height()));
}
}  // namespace

EditableLabel::EditableLabel(QWidget* parent) : QWidget(parent), m_edit(new QTextEdit(this))
{
    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_edit);
    setLayout(layout);

    m_edit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->installEventFilter(this);
    m_edit->setFrameStyle(QFrame::NoFrame);
    m_edit->setTabChangesFocus(true);

    updatePalette();
}

void EditableLabel::setText(const QString& text)
{
    m_edit->setPlainText(text);
    m_edit->setAlignment(Qt::AlignHCenter);
    m_edit->document()->clearUndoRedoStacks();
    autoResize(m_edit);
}

void EditableLabel::updatePalette()
{
    QPalette transparentPalette = palette();
    transparentPalette.setBrush(QPalette::Base, QColorConstants::Transparent);
    m_edit->setPalette(transparentPalette);
}

bool EditableLabel::eventFilter(QObject* watched, QEvent* event)
{
    Q_ASSERT(watched == m_edit);

    switch (event->type()) {
        case QEvent::FocusIn:
            m_oldText = m_edit->toPlainText();
            QTimer::singleShot(0, m_edit, &QTextEdit::selectAll);
            return false;
        case QEvent::FocusOut: {
            m_edit->setTextCursor(QTextCursor(m_edit->document()));
            m_edit->document()->clearUndoRedoStacks();
            QString newText = m_edit->toPlainText();
            if (m_oldText != newText) {
                autoResize(m_edit);
                emit textChanged(newText);
            }
            return false;
        }
        case QEvent::KeyPress: {
            const auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                m_edit->setPlainText(m_oldText);
                m_edit->setAlignment(Qt::AlignHCenter);
                m_edit->clearFocus();
                return true;
            }
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                m_edit->clearFocus();
                return true;
            }
            return false;
        }
        case QEvent::Resize:
        case QEvent::StyleChange:
            if (event->type() == QEvent::Resize) {
                const auto* resizeEvent = static_cast<QResizeEvent*>(event);
                if (resizeEvent->oldSize().width() == resizeEvent->size().width()) {
                    return false;
                }
            }

            QTimer::singleShot(0, [edit = m_edit] { autoResize(edit); });
            return false;
        case QEvent::PaletteChange:
            updatePalette();
            return false;
        default:
            return QWidget::eventFilter(watched, event);
    }
}
