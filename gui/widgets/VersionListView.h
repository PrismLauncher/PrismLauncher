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

#pragma once
#include <QTreeView>

class Mod;

class VersionListView : public QTreeView
{
	Q_OBJECT
public:
	explicit VersionListView(QWidget *parent = 0);
	virtual void paintEvent(QPaintEvent *event) override;
	void setEmptyString(QString emptyString);
	virtual void setModel ( QAbstractItemModel* model );

public slots:
	virtual void reset() override;

protected slots:
	virtual void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end) override;
	virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;

private: /* methods */
	void paintInfoLabel(QPaintEvent *event);

private: /* variables */
	int m_itemCount = 0;
	QString m_emptyString;
};
