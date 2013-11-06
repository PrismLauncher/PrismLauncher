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

#include "logic/NagUtils.h"
#include "gui/dialogs/CustomMessageBox.h"

namespace NagUtils
{
void checkJVMArgs(QString jvmargs, QWidget *parent)
{
	if (jvmargs.contains("-XX:PermSize=") || jvmargs.contains(QRegExp("-Xm[sx]")))
	{
		CustomMessageBox::selectable(
			parent, parent->tr("JVM arguments warning"),
			parent->tr("You tried to manually set a JVM memory option (using "
					   " \"-XX:PermSize\", \"-Xmx\" or \"-Xms\") - there"
					   " are dedicated boxes for these in the settings (Java"
					   " tab, in the Memory group at the top).\n"
					   "Your manual settings will be overridden by the"
					   " dedicated options.\n"
					   "This message will be displayed until you remove them"
					   " from the JVM arguments."),
			QMessageBox::Warning)->exec();
	}
}
}
