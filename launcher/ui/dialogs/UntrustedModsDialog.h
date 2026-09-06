#pragma once

#include <QDialog>
#include <QObject>
#include <QStringList>
#include <QWidget>

namespace Ui {
class UntrustedModsDialog;
}

class UntrustedModsDialog : public QDialog {
    Q_OBJECT
   public:
    explicit UntrustedModsDialog(const QStringList& paths, QWidget* parent = nullptr);
    ~UntrustedModsDialog() override;

   private:
    Ui::UntrustedModsDialog* m_ui;
};
