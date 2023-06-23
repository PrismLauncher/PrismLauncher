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

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ResourcePack.h"
#include "minecraft/mod/TexturePack.h"

namespace Ui
{
class InfoFrame;
}

class InfoFrame : public QFrame {
    Q_OBJECT

   public:
    InfoFrame(QWidget* parent = nullptr);
    ~InfoFrame() override;

    void setName(QString text = {});
    void setDescription(QString text = {});
    void setImage(QPixmap img = {});
    void setLicense(QString text = {});
    void setIssueTracker(QString text = {});

    void clear();

    void updateWithMod(Mod const& m);
    void updateWithResource(Resource const& resource);
    void updateWithResourcePack(ResourcePack& rp);
    void updateWithTexturePack(TexturePack& tp);

    static QString renderColorCodes(QString input);

   public slots:
    void descriptionEllipsisHandler(QString link);
    void licenseEllipsisHandler(QString link);
    void boxClosed(int result);

   private:
    void updateHiddenState();

   private:
    Ui::InfoFrame* ui;
    QString m_description;
    QString m_license;
    class QMessageBox* m_current_box = nullptr;
};
