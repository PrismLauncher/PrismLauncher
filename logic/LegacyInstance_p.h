/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <QString>
#include <settingsobject.h>
#include <memory>

#include "BaseInstance_p.h"
#include "ModList.h"

struct LegacyInstancePrivate : public BaseInstancePrivate
{
	std::shared_ptr<ModList> jar_mod_list;
	std::shared_ptr<ModList> core_mod_list;
	std::shared_ptr<ModList> loader_mod_list;
	std::shared_ptr<ModList> texture_pack_list;
};
