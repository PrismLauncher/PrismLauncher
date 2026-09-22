// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"

class PathsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit PathsPage(QWidget* parent = nullptr) : QWidget(parent) {}

    QString displayName() const override { return tr("Paths"); }
    QIcon icon() const override { return QIcon::fromTheme("viewfolder"); }
    QString id() const override { return "paths"; }
};
