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

#include <QDialog>

#include <memory>

#include "minecraft/auth/MojangAccountList.h"

namespace Ui
{
class AccountSelectDialog;
}

class AccountSelectDialog : public QDialog
{
	Q_OBJECT
public:
	enum Flags
	{
		NoFlags = 0,

		/*!
		 * Shows a check box on the dialog that allows the user to specify that the account
		 * they've selected should be used as the global default for all instances.
		 */
		GlobalDefaultCheckbox,

		/*!
		 * Shows a check box on the dialog that allows the user to specify that the account
		 * they've selected should be used as the default for the instance they are currently launching.
		 * This is not currently implemented.
		 */
		InstanceDefaultCheckbox,
	};

	/*!
	 * Constructs a new account select dialog with the given parent and message.
	 * The message will be shown at the top of the dialog. It is an empty string by default.
	 */
	explicit AccountSelectDialog(const QString& message="", int flags=0, QWidget *parent = 0);
	~AccountSelectDialog();

	/*!
	 * Gets a pointer to the account that the user selected.
	 * This is null if the user clicked cancel or hasn't clicked OK yet.
	 */
	MojangAccountPtr selectedAccount() const;

	/*!
	 * Returns true if the user checked the "use as global default" checkbox.
	 * If the checkbox wasn't shown, this function returns false.
	 */
	bool useAsGlobalDefault() const;

	/*!
	 * Returns true if the user checked the "use as instance default" checkbox.
	 * If the checkbox wasn't shown, this function returns false.
	 */
	bool useAsInstDefaullt() const;

public
slots:
	void on_buttonBox_accepted();

	void on_buttonBox_rejected();

protected:
	std::shared_ptr<MojangAccountList> m_accounts;

	//! The account that was selected when the user clicked OK.
	MojangAccountPtr m_selected;

private:
	Ui::AccountSelectDialog *ui;
};
