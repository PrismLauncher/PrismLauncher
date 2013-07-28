#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QDialog>
#include "MinecraftProcess.h"

namespace Ui {
class ConsoleWindow;
}

class ConsoleWindow : public QDialog
{
	Q_OBJECT

public:
	explicit ConsoleWindow(QWidget *parent = 0);
	~ConsoleWindow();

	/**
	 * @brief specify if the window is allowed to close
	 * @param mayclose
	 * used to keep it alive while MC runs
	 */
	void setMayClose(bool mayclose);

public slots:
	/**
	 * @brief write a string
	 * @param data the string
	 * @param mode the WriteMode
	 * lines have to be put through this as a whole!
	 */
	void write(QString data, MessageLevel::Enum level=MessageLevel::MultiMC);

	/**
	 * @brief write a colored paragraph
	 * @param data the string
	 * @param color the css color name
	 * this will only insert a single paragraph.
	 * \n are ignored. a real \n is always appended.
	 */
	void writeColor(QString data, const char *color=nullptr);

	/**
	 * @brief clear the text widget
	 */
	void clear();

private slots:
	void on_closeButton_clicked();

protected:
	void closeEvent(QCloseEvent *);

private:
	Ui::ConsoleWindow *ui;
	bool m_mayclose;
};

#endif // CONSOLEWINDOW_H
