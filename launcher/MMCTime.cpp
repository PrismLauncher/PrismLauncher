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

QString Time::humanReadableDuration(int64_t duration, const QString& fmt)
{
    using days = std::chrono::duration<double, std::ratio<86400>>;
    using daysR = std::chrono::duration<int, std::ratio<86400>>;
    using hours = std::chrono::duration<double, std::chrono::hours::period>;
    using minutes = std::chrono::duration<double, std::chrono::minutes::period>;
    auto std_duration = std::chrono::duration<double>(double(duration));

    QString outStr;
    QTextStream os(&outStr);

    QChar option = QChar::Null;
    bool inOption;
    QString precSeg;
    QTextStream s(&precSeg);

    QString segment;
    QTextStream seg(&segment);

    for (auto i = 0; i < fmt.size(); i++) {
        auto c = fmt[i];
        if (!inOption) {
            if (inOption = c == '%'; inOption) {
                seg.flush();
                os << segment;
                seg.reset();
                segment = "";
            } else {
                seg << c;
            }
            continue;
        }
        if (c == '%') {
            inOption = false;
            int precision = 0;
            s.flush();
            if (precSeg.startsWith('.')) {
                precision = precSeg.remove(0, 1).toInt();
            }
            s.reset();
            precSeg = "";
            double dc = 0;
            if (option == 'd') {
                auto d = std::chrono::duration_cast<days>(std_duration);
                dc = d.count();
                qDebug() << QString("%1 days").arg(d.count());
            } else if (option == 'h') {
                auto d = std::chrono::duration_cast<daysR>(std_duration);
                auto h = std::chrono::duration_cast<hours>(std_duration - d);
                dc = h.count();
                qDebug() << QString("%1 h").arg(h.count());
            } else if (option == 'H') {
                auto h = std::chrono::duration_cast<hours>(std_duration);
                dc = h.count();
            } else if (option == 'm') {
                auto d = std::chrono::duration_cast<daysR>(std_duration);
                auto h = std::chrono::duration_cast<std::chrono::hours>(std_duration - d);
                auto m = std::chrono::duration_cast<minutes>(std_duration - d - h);
                dc = m.count();
                qDebug() << QString("%1 m").arg(m.count());
            } else if (option == 'M') {
                auto m = std::chrono::duration_cast<minutes>(std_duration);
                dc = m.count();
            } else if (option == 's') {
                auto d = std::chrono::duration_cast<daysR>(std_duration);
                auto h = std::chrono::duration_cast<std::chrono::hours>(std_duration - d);
                auto m = std::chrono::duration_cast<std::chrono::minutes>(std_duration - d - h);
                auto s = std::chrono::duration_cast<std::chrono::seconds>(std_duration - d - h - m);
                qDebug() << QString("%1 s").arg(s.count());
                dc = s.count();
            } else if (option == 'S') {
                auto s = std::chrono::duration_cast<std::chrono::seconds>(std_duration);
                dc = s.count();
            }
            option = QChar::Null;
            if (dc) {
                QString formated;
                if (precision != 0)
                    formated = QString::number(dc, 'f', precision);
                else
                    formated = QString::number(int(dc));
                qDebug() << QString("%1 val").arg(formated);
                if (formated != "0") {
                    seg << formated;
                    continue;
                }
            }
            seg.reset();
            segment = "";
            for (; i + 1 < fmt.size() && fmt[i + 1] != '%'; i++)
                ;
            continue;
        }
        if (option == QChar::Null)
            option = c;
        else
            s << c;
    }
    seg.flush();
    os << segment;
    os.flush();
    return outStr.trimmed();
}