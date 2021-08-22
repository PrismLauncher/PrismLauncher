#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPair>

#include "Reply.h"
#include "RequestParameter.h"
#include "Bits.h"

namespace Katabasis {

class ReplyServer;
class PollServer;


/*
 * FIXME: this is not as simple as it should be. it squishes 4 different grant flows into one big ball of mud
 * This serves no practical purpose and simply makes the code less readable / maintainable.
 *
 * Therefore: Split this into the 4 different OAuth2 flows that people can use as authentication steps. Write tests/examples for all of them.
 */

/// Simple OAuth2 authenticator.
class OAuth2: public QObject
{
    Q_OBJECT
public:
    Q_ENUMS(GrantFlow)

public:

    struct Options {
        QString userAgent = QStringLiteral("Katabasis/1.0");
        QString redirectionUrl = QStringLiteral("http://localhost:%1");
        QString responseType = QStringLiteral("code");
        QString scope;
        QString clientIdentifier;
        QString clientSecret;
        QUrl authorizationUrl;
        QUrl accessTokenUrl;
        QVector<quint16> listenerPorts = { 0 };
    };

    /// Authorization flow types.
    enum GrantFlow {
        GrantFlowAuthorizationCode, ///< @see http://tools.ietf.org/html/draft-ietf-oauth-v2-15#section-4.1
        GrantFlowImplicit, ///< @see http://tools.ietf.org/html/draft-ietf-oauth-v2-15#section-4.2
        GrantFlowResourceOwnerPasswordCredentials,
        GrantFlowDevice ///< @see https://tools.ietf.org/html/rfc8628#section-1
    };

    /// Authorization flow.
    GrantFlow grantFlow();
    void setGrantFlow(GrantFlow value);

public:
    /// Are we authenticated?
    bool linked();

    /// Authentication token.
    QString token();

    /// Provider-specific extra tokens, available after a successful authentication
    QVariantMap extraTokens();

    /// Page content on local host after successful oauth.
    /// Provide it in case you do not want to close the browser, but display something
    QByteArray replyContent() const;
    void setReplyContent(const QByteArray &value);

public:

    // TODO: remove
    /// Resource owner username.
    /// instances with the same (username, password) share the same "linked" and "token" properties.
    QString username();
    void setUsername(const QString &value);

    // TODO: remove
    /// Resource owner password.
    /// instances with the same (username, password) share the same "linked" and "token" properties.
    QString password();
    void setPassword(const QString &value);

    // TODO: remove
    /// API key.
    QString apiKey();
    void setApiKey(const QString &value);

    // TODO: remove
    /// Allow ignoring SSL errors?
    /// E.g. SurveyMonkey fails on Mac due to SSL error. Ignoring the error circumvents the problem
    bool ignoreSslErrors();
    void setIgnoreSslErrors(bool ignoreSslErrors);

    // TODO: put in `Options`
    /// User-defined extra parameters to append to request URL
    QVariantMap extraRequestParams();
    void setExtraRequestParams(const QVariantMap &value);

    // TODO: split up the class into multiple, each implementing one OAuth2 flow
    /// Grant type (if non-standard)
    QString grantType();
    void setGrantType(const QString &value);

public:
    /// Constructor.
    /// @param  parent  Parent object.
    explicit OAuth2(Options & opts, Token & token, QObject *parent = 0, QNetworkAccessManager *manager = 0);

    /// Get refresh token.
    QString refreshToken();

    /// Get token expiration time
    QDateTime expires();

public slots:
    /// Authenticate.
    virtual void link();

    /// De-authenticate.
    virtual void unlink();

    /// Refresh token.
    bool refresh();

    /// Handle situation where reply server has opted to close its connection
    void serverHasClosed(bool paramsfound = false);

signals:
    /// Emitted when a token refresh has been completed or failed.
    void refreshFinished(QNetworkReply::NetworkError error);

    /// Emitted when client needs to open a web browser window, with the given URL.
    void openBrowser(const QUrl &url);

    /// Emitted when client can close the browser window.
    void closeBrowser();

    /// Emitted when client needs to show a verification uri and user code
    void showVerificationUriAndCode(const QUrl &uri, const QString &code, int expiresIn);

    /// Emitted when authentication/deauthentication succeeded.
    void linkingSucceeded();

    /// Emitted when authentication/deauthentication failed.
    void linkingFailed();

    void activityChanged(Activity activity);

public slots:
    /// Handle verification response.
    virtual void onVerificationReceived(QMap<QString, QString>);

protected slots:
    /// Handle completion of a token request.
    virtual void onTokenReplyFinished();

    /// Handle failure of a token request.
    virtual void onTokenReplyError(QNetworkReply::NetworkError error);

    /// Handle completion of a refresh request.
    virtual void onRefreshFinished();

    /// Handle failure of a refresh request.
    virtual void onRefreshError(QNetworkReply::NetworkError error);

    /// Handle completion of a Device Authorization Request
    virtual void onDeviceAuthReplyFinished();

protected:
    /// Build HTTP request body.
    QByteArray buildRequestBody(const QMap<QString, QString> &parameters);

    /// Set refresh token.
    void setRefreshToken(const QString &v);

    /// Set token expiration time.
    void setExpires(QDateTime v);

    /// Start polling authorization server
    void startPollServer(const QVariantMap &params, int expiresIn);

    /// Set authentication token.
    void setToken(const QString &v);

    /// Set the linked state
    void setLinked(bool v);

    /// Set extra tokens found in OAuth response
    void setExtraTokens(QVariantMap extraTokens);

    /// Set local reply server
    void setReplyServer(ReplyServer *server);

    ReplyServer * replyServer() const;

    /// Set local poll server
    void setPollServer(PollServer *server);

    PollServer * pollServer() const;

    void updateActivity(Activity activity);

protected:
    QString username_;
    QString password_;

    Options options_;

    QVariantMap extraReqParams_;
    QString apiKey_;
    QNetworkAccessManager *manager_ = nullptr;
    ReplyList timedReplies_;
    GrantFlow grantFlow_;
    QString grantType_;

protected:
    QString redirectUri_;
    Token &token_;

    // this should be part of the reply server impl
    QByteArray replyContent_;

private:
    ReplyServer *replyServer_ = nullptr;
    PollServer *pollServer_ = nullptr;
    Activity activity_ = Activity::Idle;
};

}
