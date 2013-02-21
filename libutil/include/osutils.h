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

#ifndef OSUTILS_H
#define OSUTILS_H

#include <QString>

#if defined _WIN32 | defined _WIN64
#define WINDOWS	1
#elif __APPLE__ & __MACH__
#define OSX 1
#elif __linux__
#define LINUX 1
#endif

#endif // OSUTILS_H
