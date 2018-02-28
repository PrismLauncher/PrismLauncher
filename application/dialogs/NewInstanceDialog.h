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

#pragma once

#include <QDialog>

#include "BaseVersion.h"
#include "modplatform/FtbPackDownloader.h"
#include "modplatform/PackHelpers.h"

namespace Ui
{
class NewInstanceDialog;
}

class NewInstanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewInstanceDialog(const QString & initialGroup, const QString & url = QString(), QWidget *parent = 0);
	~NewInstanceDialog();

	void updateDialogState();

	void setSelectedVersion(BaseVersionPtr version);

	QString instName() const;
	QString instGroup() const;
	QString iconKey() const;
	QUrl modpackUrl() const;
	BaseVersionPtr selectedVersion() const;

    bool isFtbModpackRequested();
    FtbPackDownloader* getFtbPackDownloader();

private
slots:
	void on_btnChangeVersion_clicked();
	void on_iconButton_clicked();
	void on_modpackBtn_clicked();
    void on_btnChooseFtbPack_clicked();
	void on_instNameTextBox_textChanged(const QString &arg1);
	void versionListUpdated();

    void ftbPackDataDownloadSuccessfully();
    void ftbPackDataDownloadFailed();

private:
	Ui::NewInstanceDialog *ui;

	bool m_versionSetByUser = false;
    bool ftbModpackRequested = false;

	BaseVersionPtr m_selectedVersion;
	QString InstIconKey;
	QString originalPlaceholderText;

    FtbPackDownloader* ftbPackDownloader;
    FtbModpack selectedPack;
};
