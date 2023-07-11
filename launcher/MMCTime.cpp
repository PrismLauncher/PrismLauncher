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

#include <QDateTime>
#include <QObject>
#include <QTextStream>
#include <chrono>

QString Time::prettifyDuration(int64_t duration)
{
    int seconds = (int)(duration % 60);
    duration /= 60;
    int minutes = (int)(duration % 60);
    duration /= 60;
    int hours = (int)(duration % 24);
    int days = (int)(duration / 24);
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

QString getOptionValue(std::chrono::duration<double> duration, char option, int precision)
{
    using days = std::chrono::duration<double, std::ratio<86400>>;
    using daysR = std::chrono::duration<int, std::ratio<86400>>;
    using hours = std::chrono::duration<double, std::chrono::hours::period>;
    using minutes = std::chrono::duration<double, std::chrono::minutes::period>;
    double dc = 0;
    switch (option) {
        case 'd': {
            auto d = std::chrono::duration_cast<days>(duration);
            dc = d.count();
            break;
        }
        case 'h': {
            auto d = std::chrono::duration_cast<daysR>(duration);
            auto h = std::chrono::duration_cast<hours>(duration - d);
            dc = h.count();
            break;
        }
        case 'H': {
            auto h = std::chrono::duration_cast<hours>(duration);
            dc = h.count();
            break;
        }
        case 'm': {
            auto d = std::chrono::duration_cast<daysR>(duration);
            auto h = std::chrono::duration_cast<std::chrono::hours>(duration - d);
            auto m = std::chrono::duration_cast<minutes>(duration - d - h);
            dc = m.count();
            break;
        }
        case 'M': {
            auto m = std::chrono::duration_cast<minutes>(duration);
            dc = m.count();
            break;
        }
        case 's': {
            auto d = std::chrono::duration_cast<daysR>(duration);
            auto h = std::chrono::duration_cast<std::chrono::hours>(duration - d);
            auto m = std::chrono::duration_cast<std::chrono::minutes>(duration - d - h);
            auto s = std::chrono::duration_cast<std::chrono::seconds>(duration - d - h - m);
            dc = s.count();
            break;
        }
        case 'S': {
            auto s = std::chrono::duration_cast<std::chrono::seconds>(duration);
            dc = s.count();
            break;
        }
        default:
            return "";
    }
    option = QChar::Null;
    if (precision != 0)
        return QString::number(dc, 'f', precision);
    return QString::number(int(dc));
}

QString Time::humanReadableDuration(int64_t duration, QString fmt, bool trimZeros)
{
    if (fmt.isEmpty())  // force default if empty
        fmt = "%dd %hh %mmin %ss";
    auto std_duration = std::chrono::duration<double>(double(duration));

    QString outStr;
    QTextStream os(&outStr);

    QString segment;
    QTextStream seg(&segment);

    // more states
    enum state { readSegment, readOption, readDot, readPrecision, readUntilNextOption };
    state current_state = readSegment;
    QChar option = QChar::Null;
    int precision = 0;

    for (auto i = 0; i < fmt.size(); i++) {
        auto c = fmt[i];
        switch (current_state) {
            case readDot: {
                if (c == '.') {
                    current_state = readPrecision;
                    break;
                }
                auto formated = getOptionValue(std_duration, option.toLatin1(), precision);
                if (formated.isEmpty() || (formated == "0" && trimZeros)) {
                    seg.reset();
                    segment = "";
                    current_state = readUntilNextOption;
                    option = QChar::Null;
                    break;
                }
                seg << formated;
                current_state = readSegment;
            }
            /* fallthrough */
            case readSegment: {
                if (c != '%') {
                    seg << c;
                    break;
                }
            }
            /* fallthrough */
            case readUntilNextOption:
                if (c == '%') {
                    current_state = readOption;
                    seg.flush();
                    os << segment;
                    seg.reset();
                    segment = "";
                }
                break;
            case readOption:
                precision = 0;
                if (c == '%') {
                    seg << "%";
                    current_state = readSegment;
                } else {
                    option = c;
                    current_state = readDot;
                }
                break;
            case readPrecision: {
                if (c.isDigit()) {
                    precision = precision * 10 + c.digitValue();
                    break;
                }
                auto formated = getOptionValue(std_duration, option.toLatin1(), precision);
                if (formated.isEmpty() || (formated == "0" && trimZeros)) {
                    seg.reset();
                    segment = "";
                    current_state = readUntilNextOption;
                    option = QChar::Null;
                    break;
                }
                seg << formated;
                if (precision == 0)
                    seg << '.';
                seg << c;
                current_state = readSegment;
                break;
            }
        }
    }
    seg.flush();
    os << segment;
    os.flush();
    outStr = outStr.trimmed();
    return outStr.isEmpty() && trimZeros ? humanReadableDuration(duration, fmt, false) : outStr;
}