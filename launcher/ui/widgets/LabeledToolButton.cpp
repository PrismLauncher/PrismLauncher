// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QLabel>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QStyleOption>
#include "LabeledToolButton.h"
#include <QApplication>
#include <QDebug>
#include "launcherlog.h"

/*
 * 
 *  Tool Button with a label on it, instead of the normal text rendering
 * 
 */

LabeledToolButton::LabeledToolButton(QWidget * parent)
    : QToolButton(parent)
    , m_label(new QLabel(this))
{
    //QToolButton::setText(" ");
    m_label->setWordWrap(true);
    m_label->setMouseTracking(false);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    // somehow, this makes word wrap work in the QLabel. yay.
    //m_label->setMinimumWidth(100);
}

QString LabeledToolButton::text() const
{
    return m_label->text();
}

void LabeledToolButton::setText(const QString & text)
{
    m_label->setText(text);
}

void LabeledToolButton::setIcon(QIcon icon)
{
    m_icon = icon;
    resetIcon();
}


/*!
    \reimp
*/
QSize LabeledToolButton::sizeHint() const
{
    /*
    Q_D(const QToolButton);
    if (d->sizeHint.isValid())
        return d->sizeHint;
    */
    ensurePolished();

    int w = 0, h = 0;
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    QSize sz =m_label->sizeHint();
    w = sz.width();
    h = sz.height();

    opt.rect.setSize(QSize(w, h)); // PM_MenuButtonIndicator depends on the height
    if (popupMode() == MenuButtonPopup)
        w += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
    
    return style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this);
}



void LabeledToolButton::resizeEvent(QResizeEvent * event)
{
    m_label->setGeometry(QRect(4, 4, width()-8, height()-8));
    if(!m_icon.isNull())
    {
        resetIcon();
    }
    QWidget::resizeEvent(event);
}

void LabeledToolButton::resetIcon()
{
    auto iconSz = m_icon.actualSize(QSize(160, 80));
    float w = iconSz.width();
    float h = iconSz.height();
    float ar = w/h;
    // FIXME: hardcoded max size of 160x80
    int newW = 80 * ar;
    if(newW > 160)
        newW = 160;
    QSize newSz (newW, 80);
    auto pixmap = m_icon.pixmap(newSz);
    m_label->setPixmap(pixmap);
    m_label->setMinimumHeight(80);
    m_label->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
}
