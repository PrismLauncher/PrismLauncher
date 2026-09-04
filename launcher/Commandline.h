/* Copyright 2013-2021 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

/**
 * @file libutil/include/cmdutils.h
 * @brief commandline parsing and processing utilities
 */

namespace Commandline {

/**
 * @brief split a string into argv items like a shell would do
 * @param args the argument string
 * @return a QStringList containing all arguments
 */
QStringList splitArgs(const QString& args);

/**
 * @brief expand variables in a string like a shell would do
 * @param input the input string
 * @param dict the environment dictionary
 * @return the expanded string
 */
QString expandVariables(const QString& input, const QProcessEnvironment& dict);

/**
 * @brief quote a single argument so that QProcess::splitCommand() parses it back out unchanged
 * @param input the argument to quote
 * @return the quoted argument, or the input unchanged if quoting isn't necessary
 */
QString quoteForSplitCommand(const QString& input);
}  // namespace Commandline
