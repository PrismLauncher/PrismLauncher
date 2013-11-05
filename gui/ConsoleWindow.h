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

#pragma once

#include <QDialog>
#include "logic/MinecraftProcess.h"

namespace Ui
{
class ConsoleWindow;
}

class ConsoleWindow : public QDialog
{
	Q_OBJECT

public:
	explicit ConsoleWindow(MinecraftProcess *proc, QWidget *parent = 0);
	~ConsoleWindow();

	/**
	 * @brief specify if the window is allowed to close
	 * @param mayclose
	 * used to keep it alive while MC runs
	 */
	void setMayClose(bool mayclose);

public
slots:
	/**
	 * @brief write a string
	 * @param data the string
	 * @param mode the WriteMode
	 * lines have to be put through this as a whole!
	 */
	void write(QString data, MessageLevel::Enum level = MessageLevel::MultiMC);

	/**
	 * @brief write a colored paragraph
	 * @param data the string
	 * @param color the css color name
	 * this will only insert a single paragraph.
	 * \n are ignored. a real \n is always appended.
	 */
	void writeColor(QString data, const char *color = nullptr);

	/**
	 * @brief clear the text widget
	 */
	void clear();

private
slots:
	void on_closeButton_clicked();
	void on_btnKillMinecraft_clicked();
	void onEnded(BaseInstance *instance);

protected:
	void closeEvent(QCloseEvent *);

private:
	Ui::ConsoleWindow *ui;
	MinecraftProcess *proc;
	bool m_mayclose;
};

