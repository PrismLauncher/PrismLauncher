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

#include <fstream>
#include <string>
#include <QDebug>

#include "PrintInstanceInfo.h"
#include <launch/LaunchTask.h>

void PrintInstanceInfo::executeTask()
{
    auto instance = m_parent->instance();
    auto lines = instance->verboseDescription(m_session);
    
#ifdef Q_OS_LINUX
    std::ifstream cpuin("/proc/cpuinfo");
    for (std::string line; std::getline(cpuin, line);)
    {
        if (strncmp(line.c_str(), "model name", 10) == 0)
        {
            QStringList clines = (QStringList() << QString::fromStdString(line.substr(13, std::string::npos)));
            logLines(clines, MessageLevel::MultiMC);
            break;
        }
    }

    char buff[512];
    int gpuline = -1;
    int cline = 0;
    FILE *fp = popen("lspci -k", "r");
    while (fgets(buff, 512, fp) != NULL)
    {
        std::string str(buff);
        if (str.substr(8, 3) == "VGA")
        {
            gpuline = cline;
            QStringList glines = (QStringList() << QString::fromStdString(str.substr(35, std::string::npos)));
            logLines(glines, MessageLevel::MultiMC);
        }
        if (gpuline > -1 && gpuline != cline)
        {
            if (cline - gpuline < 3)
            {
                QStringList alines = (QStringList() << QString::fromStdString(str.substr(1, std::string::npos)));
                logLines(alines, MessageLevel::MultiMC);
            }
        }
        cline++;
    }

    FILE *fp2 = popen("glxinfo", "r");
    while (fgets(buff, 512, fp2) != NULL)
    {
        if (strncmp(buff, "OpenGL version string:", 22) == 0)
        {
            QStringList drlines = (QStringList() << QString::fromUtf8(buff));
            logLines(drlines, MessageLevel::MultiMC);
            break;
        }
    }
#endif

    logLines(lines, MessageLevel::MultiMC);
    emitSucceeded();
}
