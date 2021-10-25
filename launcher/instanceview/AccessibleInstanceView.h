#pragma once

#include <QString>
class QAccessibleInterface;

QAccessibleInterface *groupViewAccessibleFactory(const QString &classname, QObject *object);
