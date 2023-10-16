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
#include <qobject.h>

#include <QDateTime>
#include <QObject>
#include <QTextStream>

QString Time::prettifyDuration(int64_t duration, bool noDays)
{
    int seconds = (int)(duration % 60);
    duration /= 60;
    int minutes = (int)(duration % 60);
    duration /= 60;
    int hours = (int)(noDays ? duration : (duration % 24));
    int days = (int)(noDays ? 0 : (duration / 24));
    if ((hours == 0) && (days == 0)) {
        return QObject::tr("%1min %2s").arg(minutes).arg(seconds);
    }
    if (days == 0) {
        return QObject::tr("%1h %2min").arg(hours).arg(minutes);
    }
    return QObject::tr("%1d %2h %3min").arg(days).arg(hours).arg(minutes);
}

QString Time::humanReadableDuration(double duration, int precision)
{
    using days = std::chrono::duration<int, std::ratio<86400>>;

    QString outStr;
    QTextStream os(&outStr);

    bool neg = false;
    if (duration < 0) {
        neg = true;      // flag
        duration *= -1;  // invert
    }

    auto std_duration = std::chrono::duration<double>(duration);
    auto d = std::chrono::duration_cast<days>(std_duration);
    std_duration -= d;
    auto h = std::chrono::duration_cast<std::chrono::hours>(std_duration);
    std_duration -= h;
    auto m = std::chrono::duration_cast<std::chrono::minutes>(std_duration);
    std_duration -= m;
    auto s = std::chrono::duration_cast<std::chrono::seconds>(std_duration);
    std_duration -= s;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std_duration);

    auto dc = d.count();
    auto hc = h.count();
    auto mc = m.count();
    auto sc = s.count();
    auto msc = ms.count();

    if (neg) {
        os << '-';
    }
    if (dc) {
        os << dc << QObject::tr("days");
    }
    if (hc) {
        if (dc)
            os << " ";
        os << qSetFieldWidth(2) << hc << QObject::tr("h");  // hours
    }
    if (mc) {
        if (dc || hc)
            os << " ";
        os << qSetFieldWidth(2) << mc << QObject::tr("m");  // minutes
    }
    if (dc || hc || mc || sc) {
        if (dc || hc || mc)
            os << " ";
        os << qSetFieldWidth(2) << sc << QObject::tr("s");  // seconds
    }
    if ((msc && (precision > 0)) || !(dc || hc || mc || sc)) {
        if (dc || hc || mc || sc)
            os << " ";
        os << qSetFieldWidth(0) << qSetRealNumberPrecision(precision) << msc << QObject::tr("ms");  // miliseconds
    }

    os.flush();

    return outStr;
}