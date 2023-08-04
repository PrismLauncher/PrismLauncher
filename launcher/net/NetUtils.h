// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 * Copyright (c) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QNetworkReply>
#include <QSet>

namespace Net {
inline bool isApplicationError(QNetworkReply::NetworkError x)
{
    // Mainly taken from https://github.com/qt/qtbase/blob/dev/src/network/access/qhttpthreaddelegate.cpp
    static QSet<QNetworkReply::NetworkError> errors = { QNetworkReply::ProtocolInvalidOperationError,
                                                        QNetworkReply::AuthenticationRequiredError,
                                                        QNetworkReply::ContentAccessDenied,
                                                        QNetworkReply::ContentNotFoundError,
                                                        QNetworkReply::ContentOperationNotPermittedError,
                                                        QNetworkReply::ProxyAuthenticationRequiredError,
                                                        QNetworkReply::ContentConflictError,
                                                        QNetworkReply::ContentGoneError,
                                                        QNetworkReply::InternalServerError,
                                                        QNetworkReply::OperationNotImplementedError,
                                                        QNetworkReply::ServiceUnavailableError,
                                                        QNetworkReply::UnknownServerError,
                                                        QNetworkReply::UnknownContentError };
    return errors.contains(x);
}
}  // namespace Net
