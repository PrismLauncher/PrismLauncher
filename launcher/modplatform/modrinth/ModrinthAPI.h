// SPDX-FileCopyrightText: 2022-2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "api/structures/Arguments.h"
#include "api/structures/Category.h"
#include "api/structures/ModLoader.h"
#include "modplatform/ResourceAPI.h"
#include "net/NetJob.h"

#include <QDebug>

class ModrinthAPI : public ResourceAPI {
   public:
    Platform::Provider provider() const override { return Platform::Provider::MODRINTH; }
};
