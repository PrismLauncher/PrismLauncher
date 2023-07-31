#pragma once

#include <QString>
#include <QObject>
class QAccessibleInterface;

QAccessibleInterface *groupViewAccessibleFactory(const QString &classname, QObject *object);
