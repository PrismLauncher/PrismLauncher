/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/**
  * @file platform.h
  * This file contains platform-specific functions, tweaks and fixes.
  */

#include <QWidget>

class MultiMCPlatform
{
public:
    // X11 WM_CLASS
    static void fixWM_CLASS(QWidget *widget);
};

#endif // PLATFORM_H
