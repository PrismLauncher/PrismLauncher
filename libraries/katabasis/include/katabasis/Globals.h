#pragma once

namespace Katabasis {

// Common constants
const char ENCRYPTION_KEY[] = "12345678";
const char MIME_TYPE_XFORM[] = "application/x-www-form-urlencoded";
const char MIME_TYPE_JSON[] = "application/json";

// OAuth 1/1.1 Request Parameters
const char OAUTH_CALLBACK[] = "oauth_callback";
const char OAUTH_CONSUMER_KEY[] = "oauth_consumer_key";
const char OAUTH_NONCE[] = "oauth_nonce";
const char OAUTH_SIGNATURE[] = "oauth_signature";
const char OAUTH_SIGNATURE_METHOD[] = "oauth_signature_method";
const char OAUTH_TIMESTAMP[] = "oauth_timestamp";
const char OAUTH_VERSION[] = "oauth_version";
// OAuth 1/1.1 Response Parameters
const char OAUTH_TOKEN[] = "oauth_token";
const char OAUTH_TOKEN_SECRET[] = "oauth_token_secret";
const char OAUTH_CALLBACK_CONFIRMED[] = "oauth_callback_confirmed";
const char OAUTH_VERFIER[] = "oauth_verifier";

// OAuth 2 Request Parameters
const char OAUTH2_RESPONSE_TYPE[] = "response_type";
const char OAUTH2_CLIENT_ID[] = "client_id";
const char OAUTH2_CLIENT_SECRET[] = "client_secret";
const char OAUTH2_USERNAME[] = "username";
const char OAUTH2_PASSWORD[] = "password";
const char OAUTH2_REDIRECT_URI[] = "redirect_uri";
const char OAUTH2_SCOPE[] = "scope";
const char OAUTH2_GRANT_TYPE_CODE[] = "code";
const char OAUTH2_GRANT_TYPE_TOKEN[] = "token";
const char OAUTH2_GRANT_TYPE_PASSWORD[] = "password";
const char OAUTH2_GRANT_TYPE_DEVICE[] = "urn:ietf:params:oauth:grant-type:device_code";
const char OAUTH2_GRANT_TYPE[] = "grant_type";
const char OAUTH2_API_KEY[] = "api_key";
const char OAUTH2_STATE[] = "state";
const char OAUTH2_CODE[] = "code";

// OAuth 2 Response Parameters
const char OAUTH2_ACCESS_TOKEN[] = "access_token";
const char OAUTH2_REFRESH_TOKEN[] = "refresh_token";
const char OAUTH2_EXPIRES_IN[] = "expires_in";
const char OAUTH2_DEVICE_CODE[] = "device_code";
const char OAUTH2_USER_CODE[] = "user_code";
const char OAUTH2_VERIFICATION_URI[] = "verification_uri";
const char OAUTH2_VERIFICATION_URL[] = "verification_url"; // Google sign-in
const char OAUTH2_VERIFICATION_URI_COMPLETE[] = "verification_uri_complete";
const char OAUTH2_INTERVAL[] = "interval";

// Parameter values
const char AUTHORIZATION_CODE[] = "authorization_code";

// Standard HTTP headers
const char HTTP_HTTP_HEADER[] = "HTTP";
const char HTTP_AUTHORIZATION_HEADER[] = "Authorization";

}
