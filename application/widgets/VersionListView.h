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
#include <QTreeView>

class VersionListView : public QTreeView
{
    Q_OBJECT
public:

    explicit VersionListView(QWidget *parent = 0);
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void setModel(QAbstractItemModel* model) override;

    enum EmptyMode
    {
        Empty,
        String,
        ErrorString
    };

    void setEmptyString(QString emptyString);
    void setEmptyErrorString(QString emptyErrorString);
    void setEmptyMode(EmptyMode mode);

public slots:
    virtual void reset() override;

protected slots:
    virtual void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end) override;
    virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;

private: /* methods */
    void paintInfoLabel(QPaintEvent *event) const;
    void updateEmptyViewPort();
    QString currentEmptyString() const;

private: /* variables */
    int m_itemCount = 0;
    QString m_emptyString;
    QString m_emptyErrorString;
    EmptyMode m_emptyMode = Empty;
};
