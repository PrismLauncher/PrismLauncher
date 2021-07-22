#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QDesktopServices>

#include "BuildConfig.h"

MainWindow::MainWindow(Context * context, QWidget *parent) :
    QMainWindow(parent),
    m_context(context),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    connect(m_ui->signInButton_MSA, &QPushButton::clicked, this, &MainWindow::SignInMSAClicked);
    connect(m_ui->signInButton_Mojang, &QPushButton::clicked, this, &MainWindow::SignInMojangClicked);
    connect(m_ui->signOutButton, &QPushButton::clicked, this, &MainWindow::SignOutClicked);
    connect(m_ui->refreshButton, &QPushButton::clicked, this, &MainWindow::RefreshClicked);

    // connect(m_context, &Context::linkingSucceeded, this, &MainWindow::SignInSucceeded);
    // connect(m_context, &Context::linkingFailed, this, &MainWindow::SignInFailed);
    connect(m_context, &Context::activityChanged, this, &MainWindow::ActivityChanged);
    ActivityChanged(Katabasis::Activity::Idle);
}

MainWindow::~MainWindow() = default;

void MainWindow::ActivityChanged(Katabasis::Activity activity) {
    switch(activity) {
        case Katabasis::Activity::Idle: {
            if(m_context->validity() != Katabasis::Validity::None) {
                m_ui->signInButton_Mojang->setEnabled(false);
                m_ui->signInButton_MSA->setEnabled(false);
                m_ui->signOutButton->setEnabled(true);
                m_ui->refreshButton->setEnabled(true);
                m_ui->statusBar->showMessage(QString("Hello %1!").arg(m_context->userName()));
            }
            else {
                m_ui->signInButton_Mojang->setEnabled(true);
                m_ui->signInButton_MSA->setEnabled(true);
                m_ui->signOutButton->setEnabled(false);
                m_ui->refreshButton->setEnabled(false);
                m_ui->statusBar->showMessage("Press the login button to start.");
            }
        }
        break;
        case Katabasis::Activity::LoggingIn: {
            m_ui->signInButton_Mojang->setEnabled(false);
            m_ui->signInButton_MSA->setEnabled(false);
            m_ui->signOutButton->setEnabled(false);
            m_ui->refreshButton->setEnabled(false);
            m_ui->statusBar->showMessage("Logging in...");
        }
        break;
        case Katabasis::Activity::LoggingOut: {
            m_ui->signInButton_Mojang->setEnabled(false);
            m_ui->signInButton_MSA->setEnabled(false);
            m_ui->signOutButton->setEnabled(false);
            m_ui->refreshButton->setEnabled(false);
            m_ui->statusBar->showMessage("Logging out...");
        }
        break;
        case Katabasis::Activity::Refreshing: {
            m_ui->signInButton_Mojang->setEnabled(false);
            m_ui->signInButton_MSA->setEnabled(false);
            m_ui->signOutButton->setEnabled(false);
            m_ui->refreshButton->setEnabled(false);
            m_ui->statusBar->showMessage("Refreshing login...");
        }
        break;
    }
}

void MainWindow::SignInMSAClicked() {
    qDebug() << "Sign In MSA";
    // signIn({{"prompt",  "select_account"}})
    // FIXME: wrong. very wrong. this should not be operating on the current context
    m_context->signIn();
}

void MainWindow::SignInMojangClicked() {
    qDebug() << "Sign In Mojang";
    // signIn({{"prompt",  "select_account"}})
    // FIXME: wrong. very wrong. this should not be operating on the current context
    m_context->signIn();
}


void MainWindow::SignOutClicked() {
    qDebug() << "Sign Out";
    m_context->signOut();
}

void MainWindow::RefreshClicked() {
    qDebug() << "Refresh";
    m_context->silentSignIn();
}
