#pragma once

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>

#include "Bits.h"
#include "Reply.h"
#include "RequestParameter.h"

namespace Katabasis {

class ReplyServer;
class PollServer;

/// Simple OAuth2 Device Flow authenticator.
class DeviceFlow : public QObject {
    Q_OBJECT
   public:
    Q_ENUMS(GrantFlow)

   public:
    struct Options {
        QString userAgent = QStringLiteral("Katabasis/1.0");
        QString responseType = QStringLiteral("code");
        QString scope;
        QString clientIdentifier;
        QString clientSecret;
        QUrl authorizationUrl;
        QUrl accessTokenUrl;
    };

   public:
    /// Are we authenticated?
    bool linked();

    /// Authentication token.
    QString token();

    /// Provider-specific extra tokens, available after a successful authentication
    QVariantMap extraTokens();

   public:
    // TODO: put in `Options`
    /// User-defined extra parameters to append to request URL
    QVariantMap extraRequestParams();
    void setExtraRequestParams(const QVariantMap& value);

    // TODO: split up the class into multiple, each implementing one OAuth2 flow
    /// Grant type (if non-standard)
    QString grantType();
    void setGrantType(const QString& value);

   public:
    /// Constructor.
    /// @param  parent  Parent object.
    explicit DeviceFlow(Options& opts, Token& token, QObject* parent = 0, QNetworkAccessManager* manager = 0);

    /// Get refresh token.
    QString refreshToken();

    /// Get token expiration time
    QDateTime expires();

   public slots:
    /// Authenticate.
    void login();

    /// De-authenticate.
    void logout();

    /// Refresh token.
    bool refresh();

    /// Handle situation where reply server has opted to close its connection
    void serverHasClosed(bool paramsfound = false);

   signals:
    /// Emitted when client needs to open a web browser window, with the given URL.
    void openBrowser(const QUrl& url);

    /// Emitted when client can close the browser window.
    void closeBrowser();

    /// Emitted when client needs to show a verification uri and user code
    void showVerificationUriAndCode(const QUrl& uri, const QString& code, int expiresIn);

    /// Emitted when the internal state changes
    void activityChanged(Activity activity);

   public slots:
    /// Handle verification response.
    void onVerificationReceived(QMap<QString, QString>);

   protected slots:
    /// Handle completion of a Device Authorization Request
    void onDeviceAuthReplyFinished();

    /// Handle completion of a refresh request.
    void onRefreshFinished();

    /// Handle failure of a refresh request.
    void onRefreshError(QNetworkReply::NetworkError error, QNetworkReply* reply);

   protected:
    /// Set refresh token.
    void setRefreshToken(const QString& v);

    /// Set token expiration time.
    void setExpires(QDateTime v);

    /// Start polling authorization server
    void startPollServer(const QVariantMap& params, int expiresIn);

    /// Set authentication token.
    void setToken(const QString& v);

    /// Set the linked state
    void setLinked(bool v);

    /// Set extra tokens found in OAuth response
    void setExtraTokens(QVariantMap extraTokens);

    /// Set local poll server
    void setPollServer(PollServer* server);

    PollServer* pollServer() const;

    void updateActivity(Activity activity);

   protected:
    Options options_;

    QVariantMap extraReqParams_;
    QNetworkAccessManager* manager_ = nullptr;
    ReplyList timedReplies_;
    QString grantType_;

   protected:
    Token& token_;

   private:
    PollServer* pollServer_ = nullptr;
    Activity activity_ = Activity::Idle;
};

}  // namespace Katabasis
