/*
 * Copyright 2015 Petr Mrazek <peterix@gmail.com>
 * Copyright 2021 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <MMCTime.h>

#include <QObject>

QString Time::prettifyDuration(int64_t duration) {
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    int days = (int) (duration / 24);
    if((hours == 0)&&(days == 0))
    {
        return QObject::tr("%1m %2s").arg(minutes).arg(seconds);
    }
    if (days == 0)
    {
        return QObject::tr("%1h %2m").arg(hours).arg(minutes);
    }
    return QObject::tr("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
}
