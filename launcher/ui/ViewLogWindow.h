#pragma once

#include <QMainWindow>

#include "Application.h"

class OtherLogsPage;

class ViewLogWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit ViewLogWindow(QWidget* parent = nullptr);

   signals:
    void isClosing();

   protected:
    void closeEvent(QCloseEvent*) override;

   private:
    OtherLogsPage* m_page;
};
