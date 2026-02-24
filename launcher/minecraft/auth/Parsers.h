#pragma once

#include "AccountData.h"

namespace Parsers {
bool getDateTime(const QJsonValue& value, QDateTime& out);
bool getString(const QJsonValue& value, QString& out);
bool getNumber(const QJsonValue& value, double& out);
bool getNumber(const QJsonValue& value, int64_t& out);
bool getBool(const QJsonValue& value, bool& out);

bool parseXTokenResponse(QByteArray& data, Token& output, const QString& name);
bool parseMojangResponse(QByteArray& data, Token& output);

bool parseMinecraftProfile(QByteArray& data, MinecraftProfile& output);
bool parseMinecraftProfileMojang(QByteArray& data, MinecraftProfile& output);
bool parseMinecraftEntitlements(QByteArray& data, MinecraftEntitlement& output);
bool parseRolloutResponse(QByteArray& data, bool& result);
}  // namespace Parsers
