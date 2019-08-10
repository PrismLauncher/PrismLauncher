/* Copyright 2018-2019 MultiMC Contributors
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

#include <QWidget>

namespace Ui
{
class CustomCommands;
}

class CustomCommands : public QWidget
{
    Q_OBJECT

public:
    explicit CustomCommands(QWidget *parent = 0);
    virtual ~CustomCommands();
    void initialize(bool checkable, bool checked, const QString & prelaunch, const QString & wrapper, const QString & postexit);

    bool checked() const;
    QString prelaunchCommand() const;
    QString wrapperCommand() const;
    QString postexitCommand() const;

private:
    Ui::CustomCommands *ui;
};


