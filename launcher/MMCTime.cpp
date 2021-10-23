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
