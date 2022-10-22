// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QStringList>
#include <QString>
#include <QDir>
#include <QStringList>

#include <settings/Setting.h>

#include <QDebug>
#include "java/JavaUtils.h"
#include "java/JavaInstallList.h"
#include "FileSystem.h"
#include "Application.h"

#define IBUS "@im=ibus"

JavaUtils::JavaUtils()
{
}

QString stripVariableEntries(QString name, QString target, QString remove)
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
            qWarning() << "Entry" << item
                << "could not be stripped from variable" << name;
    }
    return targetItems.join(delimiter);
}

QProcessEnvironment CleanEnviroment()
{
    // prepare the process environment
    QProcessEnvironment rawenv = QProcessEnvironment::systemEnvironment();
    QProcessEnvironment env;

    QStringList ignored =
    {
        "JAVA_ARGS",
        "CLASSPATH",
        "CONFIGPATH",
        "JAVA_HOME",
        "JRE_HOME",
        "_JAVA_OPTIONS",
        "JAVA_OPTIONS",
        "JAVA_TOOL_OPTIONS"
    };

    QStringList stripped =
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        "LD_LIBRARY_PATH",
        "LD_PRELOAD",
#endif
        "QT_PLUGIN_PATH",
        "QT_FONTPATH"
    };
    for(auto key: rawenv.keys())
    {
        auto value = rawenv.value(key);
        // filter out dangerous java crap
        if(ignored.contains(key))
        {
            qDebug() << "Env: ignoring" << key << value;
            continue;
        }

        // These are used to strip the original variables
        // If there is "LD_LIBRARY_PATH" and "LAUNCHER_LD_LIBRARY_PATH", we want to
        // remove all values in "LAUNCHER_LD_LIBRARY_PATH" from "LD_LIBRARY_PATH"
        if(key.startsWith("LAUNCHER_"))
        {
            qDebug() << "Env: ignoring" << key << value;
            continue;
        }
        if(stripped.contains(key))
        {
            QString newValue = stripVariableEntries(key, value, rawenv.value("LAUNCHER_" + key));

            qDebug() << "Env: stripped" << key << value << "to" << newValue;
        }
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        // Strip IBus
        // IBus is a Linux IME framework. For some reason, it breaks MC?
        if (key == "XMODIFIERS" && value.contains(IBUS))
        {
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
    if(!env.contains("LD_LIBRARY_PATH"))
    {
        env.insert("LD_LIBRARY_PATH", "");
    }
#endif

    return env;
}

JavaInstallPtr JavaUtils::MakeJavaPtr(QString path, QString id, QString arch)
{
    JavaInstallPtr javaVersion(new JavaInstall());

    javaVersion->id = id;
    javaVersion->arch = arch;
    javaVersion->path = path;

    return javaVersion;
}

JavaInstallPtr JavaUtils::GetDefaultJava()
{
    JavaInstallPtr javaVersion(new JavaInstall());

    javaVersion->id = "java";
    javaVersion->arch = "unknown";
#if defined(Q_OS_WIN32)
    javaVersion->path = "javaw";
#else
    javaVersion->path = "java";
#endif

    return javaVersion;
}

QStringList addJavasFromEnv(QList<QString> javas)
{
    auto env = qEnvironmentVariable("PRISMLAUNCHER_JAVA_PATHS");  // FIXME: use launcher name from buildconfig
#if defined(Q_OS_WIN32)
    QList<QString> javaPaths = env.replace("\\", "/").split(QLatin1String(";"));
    
    auto envPath = qEnvironmentVariable("PATH");
    QList<QString> javaPathsfromPath = envPath.replace("\\", "/").split(QLatin1String(";"));
    for (QString string : javaPathsfromPath) {
        javaPaths.append(string + "/javaw.exe");
    }
#else
    QList<QString> javaPaths = env.split(QLatin1String(":"));
#endif
    for(QString i : javaPaths)
    {
        javas.append(i);
    };
    return javas;
}

#if defined(Q_OS_WIN32)
QList<JavaInstallPtr> JavaUtils::FindJavaFromRegistryKey(DWORD keyType, QString keyName, QString keyJavaDir, QString subkeySuffix)
{
    QList<JavaInstallPtr> javas;

    QString archType = "unknown";
    if (keyType == KEY_WOW64_64KEY)
        archType = "64";
    else if (keyType == KEY_WOW64_32KEY)
        archType = "32";

    HKEY jreKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName.toStdWString().c_str(), 0,
                      KEY_READ | keyType | KEY_ENUMERATE_SUB_KEYS, &jreKey) == ERROR_SUCCESS)
    {
        // Read the current type version from the registry.
        // This will be used to find any key that contains the JavaHome value.

        WCHAR subKeyName[255];
        DWORD subKeyNameSize, numSubKeys, retCode;

        // Get the number of subkeys
        RegQueryInfoKeyW(jreKey, NULL, NULL, NULL, &numSubKeys, NULL, NULL, NULL, NULL, NULL,
                        NULL, NULL);

        // Iterate until RegEnumKeyEx fails
        if (numSubKeys > 0)
        {
            for (DWORD i = 0; i < numSubKeys; i++)
            {
                subKeyNameSize = 255;
                retCode = RegEnumKeyExW(jreKey, i, subKeyName, &subKeyNameSize, NULL, NULL, NULL,
                                        NULL);
                QString newSubkeyName = QString::fromWCharArray(subKeyName);
                if (retCode == ERROR_SUCCESS)
                {
                    // Now open the registry key for the version that we just got.
                    QString newKeyName = keyName + "\\" + newSubkeyName + subkeySuffix;

                    HKEY newKey;
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, newKeyName.toStdWString().c_str(), 0,
                                      KEY_READ | KEY_WOW64_64KEY, &newKey) == ERROR_SUCCESS)
                    {
                        // Read the JavaHome value to find where Java is installed.
                        DWORD valueSz = 0;
                        if (RegQueryValueExW(newKey, keyJavaDir.toStdWString().c_str(), NULL, NULL, NULL,
                                             &valueSz) == ERROR_SUCCESS)
                        {
                            WCHAR *value = new WCHAR[valueSz];
                            RegQueryValueExW(newKey, keyJavaDir.toStdWString().c_str(), NULL, NULL, (BYTE *)value,
                                             &valueSz);

                            QString newValue = QString::fromWCharArray(value);
                            delete [] value;

                            // Now, we construct the version object and add it to the list.
                            JavaInstallPtr javaVersion(new JavaInstall());

                            javaVersion->id = newSubkeyName;
                            javaVersion->arch = archType;
                            javaVersion->path =
                                QDir(FS::PathCombine(newValue, "bin")).absoluteFilePath("javaw.exe");
                            javas.append(javaVersion);
                        }

                        RegCloseKey(newKey);
                    }
                }
            }
        }

        RegCloseKey(jreKey);
    }

    return javas;
}

QList<QString> JavaUtils::FindJavaPaths()
{
    QList<JavaInstallPtr> java_candidates;

    // Oracle
    QList<JavaInstallPtr> JRE64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\Java Runtime Environment", "JavaHome");
    QList<JavaInstallPtr> JDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\Java Development Kit", "JavaHome");
    QList<JavaInstallPtr> JRE32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\Java Runtime Environment", "JavaHome");
    QList<JavaInstallPtr> JDK32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\Java Development Kit", "JavaHome");

    // Oracle for Java 9 and newer
    QList<JavaInstallPtr> NEWJRE64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\JRE", "JavaHome");
    QList<JavaInstallPtr> NEWJDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\JDK", "JavaHome");
    QList<JavaInstallPtr> NEWJRE32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\JRE", "JavaHome");
    QList<JavaInstallPtr> NEWJDK32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\JDK", "JavaHome");

    // AdoptOpenJDK
    QList<JavaInstallPtr> ADOPTOPENJRE32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\AdoptOpenJDK\\JRE", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTOPENJRE64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\AdoptOpenJDK\\JRE", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTOPENJDK32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\AdoptOpenJDK\\JDK", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTOPENJDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\AdoptOpenJDK\\JDK", "Path", "\\hotspot\\MSI");

    // Eclipse Foundation
    QList<JavaInstallPtr> FOUNDATIONJDK32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\Eclipse Foundation\\JDK", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> FOUNDATIONJDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\Eclipse Foundation\\JDK", "Path", "\\hotspot\\MSI");

    // Eclipse Adoptium
    QList<JavaInstallPtr> ADOPTIUMJRE32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\Eclipse Adoptium\\JRE", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTIUMJRE64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\Eclipse Adoptium\\JRE", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTIUMJDK32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\Eclipse Adoptium\\JDK", "Path", "\\hotspot\\MSI");
    QList<JavaInstallPtr> ADOPTIUMJDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\Eclipse Adoptium\\JDK", "Path", "\\hotspot\\MSI");

    // Microsoft
    QList<JavaInstallPtr> MICROSOFTJDK64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\Microsoft\\JDK", "Path", "\\hotspot\\MSI");

    // Azul Zulu
    QList<JavaInstallPtr> ZULU64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\Azul Systems\\Zulu", "InstallationPath");
    QList<JavaInstallPtr> ZULU32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\Azul Systems\\Zulu", "InstallationPath");

    // BellSoft Liberica
    QList<JavaInstallPtr> LIBERICA64s = this->FindJavaFromRegistryKey(
        KEY_WOW64_64KEY, "SOFTWARE\\BellSoft\\Liberica", "InstallationPath");
    QList<JavaInstallPtr> LIBERICA32s = this->FindJavaFromRegistryKey(
        KEY_WOW64_32KEY, "SOFTWARE\\BellSoft\\Liberica", "InstallationPath");

    // List x64 before x86
    java_candidates.append(JRE64s);
    java_candidates.append(NEWJRE64s);
    java_candidates.append(ADOPTOPENJRE64s);
    java_candidates.append(ADOPTIUMJRE64s);
    java_candidates.append(MakeJavaPtr("C:/Program Files/Java/jre8/bin/javaw.exe"));
    java_candidates.append(MakeJavaPtr("C:/Program Files/Java/jre7/bin/javaw.exe"));
    java_candidates.append(MakeJavaPtr("C:/Program Files/Java/jre6/bin/javaw.exe"));
    java_candidates.append(JDK64s);
    java_candidates.append(NEWJDK64s);
    java_candidates.append(ADOPTOPENJDK64s);
    java_candidates.append(FOUNDATIONJDK64s);
    java_candidates.append(ADOPTIUMJDK64s);
    java_candidates.append(MICROSOFTJDK64s);
    java_candidates.append(ZULU64s);
    java_candidates.append(LIBERICA64s);

    java_candidates.append(JRE32s);
    java_candidates.append(NEWJRE32s);
    java_candidates.append(ADOPTOPENJRE32s);
    java_candidates.append(ADOPTIUMJRE32s);
    java_candidates.append(MakeJavaPtr("C:/Program Files (x86)/Java/jre8/bin/javaw.exe"));
    java_candidates.append(MakeJavaPtr("C:/Program Files (x86)/Java/jre7/bin/javaw.exe"));
    java_candidates.append(MakeJavaPtr("C:/Program Files (x86)/Java/jre6/bin/javaw.exe"));
    java_candidates.append(JDK32s);
    java_candidates.append(NEWJDK32s);
    java_candidates.append(ADOPTOPENJDK32s);
    java_candidates.append(FOUNDATIONJDK32s);
    java_candidates.append(ADOPTIUMJDK32s);
    java_candidates.append(ZULU32s);
    java_candidates.append(LIBERICA32s);

    java_candidates.append(MakeJavaPtr(this->GetDefaultJava()->path));

    QList<QString> candidates;
    for(JavaInstallPtr java_candidate : java_candidates)
    {
        if(!candidates.contains(java_candidate->path))
        {
            candidates.append(java_candidate->path);
        }
    }

    candidates = addJavasFromEnv(candidates);
    candidates.removeDuplicates();
    return candidates;
}

#elif defined(Q_OS_MAC)
QList<QString> JavaUtils::FindJavaPaths()
{
    QList<QString> javas;
    javas.append(this->GetDefaultJava()->path);
    javas.append("/Applications/Xcode.app/Contents/Applications/Application Loader.app/Contents/MacOS/itms/java/bin/java");
    javas.append("/Library/Internet Plug-Ins/JavaAppletPlugin.plugin/Contents/Home/bin/java");
    javas.append("/System/Library/Frameworks/JavaVM.framework/Versions/Current/Commands/java");
    QDir libraryJVMDir("/Library/Java/JavaVirtualMachines/");
    QStringList libraryJVMJavas = libraryJVMDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &java, libraryJVMJavas) {
        javas.append(libraryJVMDir.absolutePath() + "/" + java + "/Contents/Home/bin/java");
        javas.append(libraryJVMDir.absolutePath() + "/" + java + "/Contents/Home/jre/bin/java");
    }
    QDir systemLibraryJVMDir("/System/Library/Java/JavaVirtualMachines/");
    QStringList systemLibraryJVMJavas = systemLibraryJVMDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &java, systemLibraryJVMJavas) {
        javas.append(systemLibraryJVMDir.absolutePath() + "/" + java + "/Contents/Home/bin/java");
        javas.append(systemLibraryJVMDir.absolutePath() + "/" + java + "/Contents/Commands/java");
    }
    javas = addJavasFromEnv(javas);
    javas.removeDuplicates();
    return javas;
}

#elif defined(Q_OS_LINUX)
QList<QString> JavaUtils::FindJavaPaths()
{
    qDebug() << "Linux Java detection incomplete - defaulting to \"java\"";

    QList<QString> javas;
    javas.append(this->GetDefaultJava()->path);
    auto scanJavaDir = [&](const QString & dirPath)
    {
        QDir dir(dirPath);
        if(!dir.exists())
            return;
        auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        for(auto & entry: entries)
        {

            QString prefix;
            if(entry.isAbsolute())
            {
                prefix = entry.absoluteFilePath();
            }
            else
            {
                prefix = entry.filePath();
            }

            javas.append(FS::PathCombine(prefix, "jre/bin/java"));
            javas.append(FS::PathCombine(prefix, "bin/java"));
        }
    };
    // oracle RPMs
    scanJavaDir("/usr/java");
    // general locations used by distro packaging
    scanJavaDir("/usr/lib/jvm");
    scanJavaDir("/usr/lib64/jvm");
    scanJavaDir("/usr/lib32/jvm");
    // javas stored in Prism Launcher's folder
    scanJavaDir("java");
    // manually installed JDKs in /opt
    scanJavaDir("/opt/jdk");
    scanJavaDir("/opt/jdks");
    // flatpak
    scanJavaDir("/app/jdk");
    javas = addJavasFromEnv(javas);
    javas.removeDuplicates();
    return javas;
}
#else
QList<QString> JavaUtils::FindJavaPaths()
{
    qDebug() << "Unknown operating system build - defaulting to \"java\"";

    QList<QString> javas;
    javas.append(this->GetDefaultJava()->path);

    return addJavasFromEnv(javas);
}
#endif

QString JavaUtils::getJavaCheckPath()
{
    return APPLICATION->getJarPath("JavaCheck.jar");
}
