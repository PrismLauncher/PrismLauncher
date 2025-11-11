#include <QCloseEvent>

#include "ViewLogWindow.h"

#include "ui/pages/instance/OtherLogsPage.h"

ViewLogWindow::ViewLogWindow(QWidget* parent)
    : QMainWindow(parent), m_page(new OtherLogsPage("launcher-logs", tr("Launcher Logs"), "Launcher-Logs", nullptr, parent))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon::fromTheme("log"));
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
