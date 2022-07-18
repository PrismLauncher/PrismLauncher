#pragma once

#include <QButtonGroup>
#include <QDialog>

namespace Ui {
class ChooseProviderDialog;
}

namespace ModPlatform {
enum class Provider;
}

class Mod;
class NetJob;
class ModUpdateDialog;

class ChooseProviderDialog : public QDialog {
    Q_OBJECT

    struct Response {
        bool skip_all = false;
        bool confirm_all = false;

        bool try_others = false;

        ModPlatform::Provider chosen;
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

    auto getSelectedProvider() const -> ModPlatform::Provider;

   private:
    Ui::ChooseProviderDialog* ui;

    QButtonGroup m_providers;

    Response m_response;
};
