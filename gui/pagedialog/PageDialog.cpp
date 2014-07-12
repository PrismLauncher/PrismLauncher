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

#include "PageDialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>

#include "MultiMC.h"
#include "logic/settings/SettingsObject.h"
#include "gui/Platform.h"
#include "gui/widgets/IconLabel.h"
#include "gui/widgets/PageContainer.h"

PageDialog::PageDialog(BasePageProviderPtr pageProvider, QString defaultId, QWidget *parent)
	: QDialog(parent)
{
	MultiMCPlatform::fixWM_CLASS(this);
	setWindowTitle(pageProvider->dialogTitle());
	m_container = new PageContainer(pageProvider, defaultId, this);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_container);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(mainLayout);

	QDialogButtonBox *buttons =
		new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Close);
	buttons->button(QDialogButtonBox::Close)->setDefault(true);
	m_container->addButtons(buttons);

	connect(buttons->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close()));
	connect(buttons->button(QDialogButtonBox::Help), SIGNAL(clicked()), m_container,
			SLOT(help()));

	restoreGeometry(
		QByteArray::fromBase64(MMC->settings()->get("PagedGeometry").toByteArray()));
}

void PageDialog::closeEvent(QCloseEvent *event)
{
	if (m_container->requestClose(event))
	{
		MMC->settings()->set("PagedGeometry", saveGeometry().toBase64());
		QDialog::closeEvent(event);
	}
}
