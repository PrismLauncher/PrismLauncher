/* Copyright 2015 MultiMC Contributors
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

#include "pages/BasePage.h"

namespace Ui {
class WonkoPage;
}

class QSortFilterProxyModel;
class VersionProxyModel;

class WonkoPage : public QWidget, public BasePage
{
	Q_OBJECT
public:
	explicit WonkoPage(QWidget *parent = 0);
	~WonkoPage();

	QString id() const override { return "wonko-global"; }
	QString displayName() const override { return tr("Wonko"); }
	QIcon icon() const override;
	void opened() override;

private slots:
	void on_refreshIndexBtn_clicked();
	void on_refreshFileBtn_clicked();
	void on_refreshVersionBtn_clicked();
	void on_fileSearchEdit_textChanged(const QString &search);
	void on_versionSearchEdit_textChanged(const QString &search);
	void updateCurrentVersionList(const QModelIndex &index);
	void versionListDataChanged(const QModelIndex &tl, const QModelIndex &br);

private:
	Ui::WonkoPage *ui;
	QSortFilterProxyModel *m_fileProxy;
	QSortFilterProxyModel *m_filterProxy;
	VersionProxyModel *m_versionProxy;

	void updateVersion();
};
