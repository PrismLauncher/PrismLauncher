/* Copyright 2014 MultiMC Contributors
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
#include <QModelIndex>
#include <gui/pages/BasePageProvider.h>

class IconLabel;
class QSortFilterProxyModel;
class PageModel;
class QLabel;
class QListView;
class QLineEdit;
class QStackedLayout;

class PageDialog : public QDialog
{
	Q_OBJECT
public:
	explicit PageDialog(BasePageProviderPtr pageProvider, QWidget *parent = 0);
	virtual ~PageDialog() {};
private:
	void createUI();
private slots:
	void apply();
	void currentChanged(const QModelIndex &current);
	void showPage(int row);

private:
	QSortFilterProxyModel *m_proxyModel;
	PageModel *m_model;
	QStackedLayout *m_pageStack;
	QLineEdit *m_filter;
	QListView *m_pageList;
	QLabel *m_header;
	IconLabel *m_iconHeader;
};
