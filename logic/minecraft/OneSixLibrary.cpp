/* Copyright 2013-2014 MultiMC Contributors
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

#include <QJsonArray>

#include "OneSixLibrary.h"
#include "OneSixRule.h"
#include "OpSys.h"
#include "logic/net/URLConstants.h"
#include <pathutils.h>
#include <JlCompress.h>
#include "logger/QsLog.h"

OneSixLibrary::OneSixLibrary(RawLibraryPtr base)
{
	m_name = base->m_name;
	m_base_url = base->m_base_url;
	m_hint = base->m_hint;
	m_absolute_url = base->m_absolute_url;
	extract_excludes = base->extract_excludes;
	m_native_classifiers = base->m_native_classifiers;
	m_rules = base->m_rules;
	dependType = base->dependType;
	// these only make sense for raw libraries. OneSix
	/*
	insertType = base->insertType;
	insertData = base->insertData;
	*/
}

OneSixLibraryPtr OneSixLibrary::fromRawLibrary(RawLibraryPtr lib)
{
	return OneSixLibraryPtr(new OneSixLibrary(lib));
}
