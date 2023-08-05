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

#include <QFrame>

namespace Ui {
class ErrorFrame;
}

class ErrorFrame : public QFrame {
    Q_OBJECT

   public:
    explicit ErrorFrame(QWidget* parent = 0);
    ~ErrorFrame();

    void setTitle(QString text);
    void setDescription(QString text);

    void clear();

   public slots:
    void ellipsisHandler(const QString& link);
    void boxClosed(int result);

   private:
    void updateHiddenState();

   private:
    Ui::ErrorFrame* ui;
    QString desc;
    class QMessageBox* currentBox = nullptr;
};
