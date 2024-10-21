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

#include "CleanEnvironment.h"
#include <QDebug>

constexpr char IBUS[] = "@im=ibus";

static QString stripVariableEntries(QString name, QString target, QString remove)
{
    char delimiter = ':';
#ifdef Q_OS_WIN32
    delimiter = ';';
#endif

    auto targetItems = target.split(delimiter);
    auto toRemove = remove.split(delimiter);

    for (QString item : toRemove) {
        bool removed = targetItems.removeOne(item);
        if (!removed)
            qWarning() << "Entry" << item << "could not be stripped from variable" << name;
    }
    return targetItems.join(delimiter);
}

QProcessEnvironment cleanEnvironment()
{
    // prepare the process environment
    QProcessEnvironment rawenv = QProcessEnvironment::systemEnvironment();
    QProcessEnvironment env;

    QStringList ignored = { "JAVA_ARGS", "CLASSPATH",     "CONFIGPATH",   "JAVA_HOME",
                            "JRE_HOME",  "_JAVA_OPTIONS", "JAVA_OPTIONS", "JAVA_TOOL_OPTIONS" };

    QStringList stripped = {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        "LD_LIBRARY_PATH", "LD_PRELOAD",
#endif
        "QT_PLUGIN_PATH", "QT_FONTPATH"
    };
    for (auto key : rawenv.keys()) {
        auto value = rawenv.value(key);
        // filter out dangerous java crap
        if (ignored.contains(key)) {
            qDebug() << "Env: ignoring" << key << value;
            continue;
        }

        // These are used to strip the original variables
        // If there is "LD_LIBRARY_PATH" and "LAUNCHER_LD_LIBRARY_PATH", we want to
        // remove all values in "LAUNCHER_LD_LIBRARY_PATH" from "LD_LIBRARY_PATH"
        if (key.startsWith("LAUNCHER_")) {
            qDebug() << "Env: ignoring" << key << value;
            continue;
        }
        if (stripped.contains(key)) {
            QString newValue = stripVariableEntries(key, value, rawenv.value("LAUNCHER_" + key));

            qDebug() << "Env: stripped" << key << value << "to" << newValue;
        }
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        // Strip IBus
        // IBus is a Linux IME framework. For some reason, it breaks MC?
        if (key == "XMODIFIERS" && value.contains(IBUS)) {
            QString save = value;
            value.replace(IBUS, "");
            qDebug() << "Env: stripped" << IBUS << "from" << save << ":" << value;
        }
#endif
        // qDebug() << "Env: " << key << value;
        env.insert(key, value);
    }
#ifdef Q_OS_LINUX
    // HACK: Workaround for QTBUG-42500
    if (!env.contains("LD_LIBRARY_PATH")) {
        env.insert("LD_LIBRARY_PATH", "");
    }
#endif

    return env;
}
