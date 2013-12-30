#include "main.h"

#include <QApplication>
#include <QStandardItemModel>
#include <QPainter>
#include <QTime>

#include "CategorizedView.h"
#include "CategorizedProxyModel.h"
#include "InstanceDelegate.h"

Progresser *progresser;

QPixmap icon(const Qt::GlobalColor color)
{
	QPixmap p = QPixmap(32, 32);
	p.fill(QColor(color));
	return p;
}
QPixmap icon(const int number)
{
	QPixmap p = icon(Qt::white);
	QPainter painter(&p);
	QFont font = painter.font();
	font.setBold(true);
	font.setPixelSize(28);
	painter.setFont(font);
	painter.drawText(QRect(QPoint(0, 0), p.size()), Qt::AlignVCenter | Qt::AlignHCenter, QString::number(number));
	painter.end();
	return p;
}
QStandardItem *createItem(const Qt::GlobalColor color, const QString &text, const QString &category)
{
	QStandardItem *item = new QStandardItem;
	item->setText(text);
	item->setData(icon(color), Qt::DecorationRole);
	item->setData(category, CategorizedViewRoles::CategoryRole);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	//progresser->addTrackedIndex(item);
	return item;
}
QStandardItem *createItem(const int index, const QString &category)
{
	QStandardItem *item = new QStandardItem;
	item->setText(QString("Item #%1").arg(index));
	item->setData(icon(index), Qt::DecorationRole);
	item->setData(category, CategorizedViewRoles::CategoryRole);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	//progresser->addTrackedIndex(item);
	return item;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	qsrand(QTime::currentTime().msec());

	progresser = new Progresser();

	QStandardItemModel model;
	model.setRowCount(10);
	model.setColumnCount(1);

	model.setItem(0, createItem(Qt::red, "Red is a color. Some more text. I'm out of ideas. 42. What's your name?", "Colorful"));
	model.setItem(1, createItem(Qt::blue, "Blue", "Colorful"));
	model.setItem(2, createItem(Qt::yellow, "Yellow", "Colorful"));

	model.setItem(3, createItem(Qt::black, "Black", "Not Colorful"));
	model.setItem(4, createItem(Qt::darkGray, "Dark Gray", "Not Colorful"));
	model.setItem(5, createItem(Qt::gray, "Gray", "Not Colorful"));
	model.setItem(6, createItem(Qt::lightGray, "Light Gray", "Not Colorful"));
	model.setItem(7, createItem(Qt::white, "White", "Not Colorful"));

	model.setItem(8, createItem(Qt::darkGreen, "Dark Green", ""));
	model.setItem(9, progresser->addTrackedIndex(createItem(Qt::green, "Green", "")));

	for (int i = 0; i < 20; ++i)
	{
		model.setItem(i + 10, createItem(i+1, "Items 1-20"));
	}

	CategorizedProxyModel pModel;
	pModel.setSourceModel(&model);

	CategorizedView w;
	w.setItemDelegate(new ListViewDelegate);
	w.setModel(&pModel);
	w.resize(640, 480);
	w.show();

	return a.exec();
}
