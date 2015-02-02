/* Copyright 2013-2015 MultiMC Contributors
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

#include <QLabel>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QStyleOption>
#include "LabeledToolButton.h"
#include <QApplication>

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
	m_label->setMinimumWidth(100);
}

QString LabeledToolButton::text() const
{
	return m_label->text();
}

void LabeledToolButton::setText(const QString & text)
{
	m_label->setText(text);
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
	
	QSize rawSize = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this);
	QSize sizeHint = rawSize.expandedTo(QApplication::globalStrut());
	return sizeHint;
}



void LabeledToolButton::resizeEvent(QResizeEvent * event)
{
	m_label->setGeometry(QRect(4, 4, width()-8, height()-8));
	QWidget::resizeEvent(event);
}
