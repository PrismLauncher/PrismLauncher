#pragma once

#include <QMainWindow>
#include <QScopedPointer>
#include <QtNetwork>
#include <katabasis/Bits.h>

#include "context.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Context * context, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void SignInMojangClicked();
    void SignInMSAClicked();

    void SignOutClicked();
    void RefreshClicked();

    void ActivityChanged(Katabasis::Activity activity);

private:
    Context* m_context;
    QScopedPointer<Ui::MainWindow> m_ui;
};

