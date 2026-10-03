#pragma once

#include "AccountData.h"
#include "Result.h"

namespace Parsers {
bool getDateTime(QJsonValue value, QDateTime& out);
bool getString(QJsonValue value, QString& out);
bool getNumber(QJsonValue value, double& out);
bool getNumber(QJsonValue value, int64_t& out);
bool getBool(QJsonValue value, bool& out);

bool parseXTokenResponse(const QByteArray& data, Token& output, QString name);
bool parseMojangResponse(const QByteArray& data, Token& output);

bool parseMinecraftProfile(const QByteArray& data, MinecraftProfile& output);
Result<> parseMinecraftProfileMojang(const QByteArray& data, MinecraftProfile& output);
bool parseMinecraftEntitlements(const QByteArray& data, MinecraftEntitlement& output);
}  // namespace Parsers
