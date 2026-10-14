// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QDialog>

// First step of creating an instance: choose between starting fresh, browsing
// modpacks across providers, or importing an existing pack.
class CreateHubDialog : public QDialog {
    Q_OBJECT

   public:
    enum class Action { StartFresh, BrowsePacks, Import };

    explicit CreateHubDialog(QWidget* parent = nullptr);
    ~CreateHubDialog() override = default;

    Action chosenAction() const { return m_action; }

   private:
    Action m_action = Action::StartFresh;
};
