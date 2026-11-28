#pragma once
#include <QWidget>

#include "CornerDecor.h"

// Приветствие "Добрый день" убрано.
// Функция оставлена, чтобы Application.cpp менять не пришлось: она только ставит украшения в углу.
inline void showGreeting(QWidget* parent)
{
    installCornerDecor(parent);
}
