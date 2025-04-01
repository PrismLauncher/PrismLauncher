#pragma once

#include <QDialog>
#include "minecraft/MinecraftInstance.h"

namespace Ui {
class ModrinthPizzaDialog;
}

class ModrinthPizzaDialog : public QDialog {
   public:
    ModrinthPizzaDialog(MinecraftInstancePtr instance, QWidget* parent = nullptr);
    virtual ~ModrinthPizzaDialog();

    void done(int) override;

    void updateFlavour();

   private:
    Ui::ModrinthPizzaDialog* m_ui;
    InstancePtr m_instance;
    long m_seed;
};