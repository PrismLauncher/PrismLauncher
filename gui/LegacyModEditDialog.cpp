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

#include "LegacyModEditDialog.h"
#include "ui_LegacyModEditDialog.h"

LegacyModEditDialog::LegacyModEditDialog( LegacyInstance* inst, QWidget* parent ) :
	m_inst(inst),
	QDialog(parent),
	ui(new Ui::LegacyModEditDialog)
{
	ui->setupUi(this);
	
}

LegacyModEditDialog::~LegacyModEditDialog()
{
	delete ui;
}

void LegacyModEditDialog::on_buttonBox_rejected()
{
	close();
}