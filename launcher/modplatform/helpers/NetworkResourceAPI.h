// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>
#include "modplatform/ResourceAPI.h"

class NetworkResourceAPI : public ResourceAPI {
   public:
    Task::Ptr getProject(QString addonId, std::shared_ptr<QByteArray> response) const override;
    Task::Ptr getDependencyVersion(DependencySearchArgs&&, DependencySearchCallbacks&&) const override;
};
