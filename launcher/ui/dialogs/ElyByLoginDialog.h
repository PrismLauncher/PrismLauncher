#pragma once

#include <QDialog>

#include "minecraft/auth/MinecraftAccount.h"

class QLineEdit;

class ElyByLoginDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ElyByLoginDialog(QWidget* parent = nullptr);
    ~ElyByLoginDialog() override = default;

    static MinecraftAccountPtr newAccount(QWidget* parent = nullptr);

   private slots:
    void tryLogin();

   private:
    QLineEdit* m_login = nullptr;
    QLineEdit* m_password = nullptr;
};
