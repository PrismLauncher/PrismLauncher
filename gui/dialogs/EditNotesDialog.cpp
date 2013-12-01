/* Copyright 2013 MultiMC Contributors
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

#include "EditNotesDialog.h"
#include "ui_EditNotesDialog.h"
#include "gui/Platform.h"

#include <QIcon>
#include <QApplication>

EditNotesDialog::EditNotesDialog(QString notes, QString name, QWidget *parent)
	: QDialog(parent), ui(new Ui::EditNotesDialog), m_instance_name(name),
	  m_instance_notes(notes)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	ui->noteEditor->setText(notes);
	setWindowTitle(tr("Edit notes of %1").arg(m_instance_name));
	// connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));
}

EditNotesDialog::~EditNotesDialog()
{
	delete ui;
}

QString EditNotesDialog::getText()
{
	return ui->noteEditor->toPlainText();
}
