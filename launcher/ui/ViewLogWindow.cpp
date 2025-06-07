#include <QCloseEvent>

#include "ViewLogWindow.h"

#include "ui/pages/instance/LogsPage.h"

ViewLogWindow::ViewLogWindow(QWidget* parent) : QMainWindow(parent), m_page(new LogsPage(nullptr, parent))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(APPLICATION->getThemedIcon("log"));
    setWindowTitle(tr("View Launcher Logs"));
    setCentralWidget(m_page);
    setMinimumSize(m_page->size());
    setContentsMargins(0, 0, 0, 0);
    m_page->opened();
    show();
}

void ViewLogWindow::closeEvent(QCloseEvent* event)
{
    m_page->closed();
    emit isClosing();
    event->accept();
}
