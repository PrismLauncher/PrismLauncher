/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "include/keyring.h"

#include "osutils.h"

#include "stubkeyring.h"

// system specific keyrings
/*#if defined(OSX)
class OSXKeychain;
#define KEYRING OSXKeychain
#elif defined(LINUX)
class XDGKeyring;
#define KEYRING XDGKeyring
#elif defined(WINDOWS)
class Win32Keystore;
#define KEYRING Win32Keystore
#else
#pragma message Keyrings are not supported on your os. Falling back to the insecure StubKeyring
#endif*/

Keyring *Keyring::instance()
{
	if (m_instance == nullptr)
	{
#ifdef KEYRING
		m_instance = new KEYRING();
		if (!m_instance->isValid())
		{
			qWarning("Could not create SystemKeyring! falling back to StubKeyring.");
			m_instance = new StubKeyring();
		}
#else
		qWarning("Keyrings are not supported on your OS. Fallback StubKeyring is insecure!");
		m_instance = new StubKeyring();
#endif
		atexit(Keyring::destroy);
	}
	return m_instance;
}

void Keyring::destroy()
{
	delete m_instance;
}

Keyring *Keyring::m_instance;
