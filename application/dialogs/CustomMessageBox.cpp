/* Copyright 2013-2018 MultiMC Contributors
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

#include "CustomMessageBox.h"

namespace CustomMessageBox
{
QMessageBox *selectable(QWidget *parent, const QString &title, const QString &text,
                        QMessageBox::Icon icon, QMessageBox::StandardButtons buttons,
                        QMessageBox::StandardButton defaultButton)
{
    QMessageBox *messageBox = new QMessageBox(parent);
    messageBox->setWindowTitle(title);
    messageBox->setText(text);
    messageBox->setStandardButtons(buttons);
    messageBox->setDefaultButton(defaultButton);
    messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageBox->setIcon(icon);
    messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);

    return messageBox;
}
}
