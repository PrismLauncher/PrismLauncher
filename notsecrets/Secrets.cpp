#include "Secrets.h"

#include <array>
#include <cstdio>

namespace {

/*
 * This is the MSA client ID. It is confidential and should not be reused.
 * You can obtain one for yourself by using azure app registration:
 * https://docs.microsoft.com/en-us/azure/active-directory/develop/quickstart-register-app
 *
 * The app registration should:
 * - Be only for personal accounts.
 * - Not have any redirect URI.
 * - Not have any platform.
 * - Have no credentials.
 * - No certificates.
 * - No client secrets.
 * - Enable 'Live SDK support' for access to XBox APIs.
 * - Enable 'public client flows' for OAuth2 device flow.
 *
 * By putting one in here, you accept the terms and conditions for using the MS Identity Plaform and assume all responsibilities associated with it.
 * See: https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use
 *
 * If you intend to base your own launcher on this code, take care and customize this to obfuscate the client ID, so it cannot be trivially found by casual attackers.
 */

QString MSAClientID = "";
}

namespace Secrets {
bool hasMSAClientID() {
    return !MSAClientID.isEmpty();
}

QString getMSAClientID(uint8_t separator) {
    return MSAClientID;
}
}
