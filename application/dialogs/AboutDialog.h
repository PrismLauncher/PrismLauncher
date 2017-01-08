/* Copyright 2013-2017 MultiMC Contributors
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
#include <net/NetJob.h>

namespace Ui
{
class AboutDialog;
}

class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AboutDialog(QWidget *parent = 0);
	~AboutDialog();

public
slots:
	/// Starts loading a list of Patreon patrons.
	void loadPatronList();
	
	/// Slot for when the patron list loads successfully.
	void patronListLoaded();

private:
	Ui::AboutDialog *ui;

	NetJobPtr netJob;
	QByteArray dataSink;
};
