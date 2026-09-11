#pragma once

#include <QButtonGroup>
#include <QDialog>
#include "modplatform/ModIndex.h"

namespace Ui {
class ChooseProviderDialog;
}

class Mod;
class NetJob;

class ChooseProviderDialog : public QDialog {
    Q_OBJECT

    struct Response {
        bool skip_all = false;
        bool confirm_all = false;

        bool try_others = false;

        ModPlatform::ResourceProvider chosen;
    };

   public:
    explicit ChooseProviderDialog(QWidget* parent, bool single_choice = false, bool allow_skipping = true);
    ~ChooseProviderDialog();

    auto getResponse() const -> Response { return m_response; }

    void setDescription(QString desc);

   private slots:
    void skipOne();
    void skipAll();
    void confirmOne();
    void confirmAll();

   private:
    void addProviders();
    void disableInput();

    auto getSelectedProvider() const -> ModPlatform::ResourceProvider;

   private:
    Ui::ChooseProviderDialog* ui;

    QButtonGroup m_providers;

    Response m_response;
};
