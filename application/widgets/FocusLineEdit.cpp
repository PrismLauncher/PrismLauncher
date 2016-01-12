#include "FocusLineEdit.h"
#include <QDebug>

FocusLineEdit::FocusLineEdit(QWidget *parent) : QLineEdit(parent)
{
	_selectOnMousePress = false;
}

void FocusLineEdit::focusInEvent(QFocusEvent *e)
{
	QLineEdit::focusInEvent(e);
	selectAll();
	_selectOnMousePress = true;
}

void FocusLineEdit::mousePressEvent(QMouseEvent *me)
{
	QLineEdit::mousePressEvent(me);
	if (_selectOnMousePress)
	{
		selectAll();
		_selectOnMousePress = false;
	}
	qDebug() << selectedText();
}
