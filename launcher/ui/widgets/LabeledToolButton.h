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

#pragma once

#include <QPushButton>
#include <QToolButton>

class QLabel;

class LabeledToolButton : public QToolButton
{
    Q_OBJECT

    QLabel * m_label;
    QIcon m_icon;

public:
    LabeledToolButton(QWidget * parent = 0);

    QString text() const;
    void setText(const QString & text);
    void setIcon(QIcon icon);
    virtual QSize sizeHint() const;
protected:
    void resizeEvent(QResizeEvent * event);
    void resetIcon();
};
