// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <memory>
#include "api/structures/ModLoader.h"
#include "api/structures/Provider.h"
#include "modplatform/ResourceAPI.h"
#include "net/NetJob.h"

class FlameAPI : public ResourceAPI {
   public:
    Platform::Provider provider() const override { return Platform::Provider::FLAME; }
};
