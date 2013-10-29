#pragma once

#include <QMessageBox>

namespace CustomMessageBox
{
	QMessageBox *selectable(QWidget *parent, const QString &title, const QString &text,
							QMessageBox::Icon icon = QMessageBox::NoIcon,
							QMessageBox::StandardButtons buttons = QMessageBox::Ok,
							QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
}
