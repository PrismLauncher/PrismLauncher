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
    setContentsMargins(6, 6, 0, 6);  // the "Other Logs" instance page has 6px padding on the right,
                                     // to have equal padding in all directions in the dialog we add it to all other sides.
    m_page->opened();
    show();
}

void ViewLogWindow::closeEvent(QCloseEvent* event)
{
    m_page->closed();
    emit isClosing();
    event->accept();
}
