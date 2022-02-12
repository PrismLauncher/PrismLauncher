#include "MSAStep.h"

#include <QNetworkRequest>

#include "BuildConfig.h"
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

#include "Application.h"

using OAuth2 = Katabasis::DeviceFlow;
using Activity = Katabasis::Activity;

MSAStep::MSAStep(AccountData* data, Action action) : AuthStep(data), m_action(action) {
    OAuth2::Options opts;
    opts.scope = "XboxLive.signin offline_access";
    opts.clientIdentifier = APPLICATION->getMSAClientID();
    opts.authorizationUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";
    opts.accessTokenUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";

    // FIXME: OAuth2 is not aware of our fancy shared pointers
    m_oauth2 = new OAuth2(opts, m_data->msaToken, this, APPLICATION->network().get());

    connect(m_oauth2, &OAuth2::activityChanged, this, &MSAStep::onOAuthActivityChanged);
    connect(m_oauth2, &OAuth2::showVerificationUriAndCode, this, &MSAStep::showVerificationUriAndCode);
}

MSAStep::~MSAStep() noexcept = default;

QString MSAStep::describe() {
    return tr("Logging in with Microsoft account.");
}


void MSAStep::rehydrate() {
    switch(m_action) {
        case Refresh: {
            // TODO: check the tokens and see if they are old (older than a day)
            return;
        }
        case Login: {
            // NOOP
            return;
        }
    }
}

void MSAStep::perform() {
    switch(m_action) {
        case Refresh: {
            m_oauth2->refresh();
            return;
        }
        case Login: {
            QVariantMap extraOpts;
            extraOpts["prompt"] = "select_account";
            m_oauth2->setExtraRequestParams(extraOpts);

            *m_data = AccountData();
            m_oauth2->login();
            return;
        }
    }
}

void MSAStep::onOAuthActivityChanged(Katabasis::Activity activity) {
    switch(activity) {
        case Katabasis::Activity::Idle:
        case Katabasis::Activity::LoggingIn:
        case Katabasis::Activity::Refreshing:
        case Katabasis::Activity::LoggingOut: {
            // We asked it to do something, it's doing it. Nothing to act upon.
            return;
        }
        case Katabasis::Activity::Succeeded: {
            // Succeeded or did not invalidate tokens
            emit hideVerificationUriAndCode();
            QVariantMap extraTokens = m_oauth2->extraTokens();
#ifndef NDEBUG
            if (!extraTokens.isEmpty()) {
                qDebug() << "Extra tokens in response:";
                foreach (QString key, extraTokens.keys()) {
                    qDebug() << "\t" << key << ":" << extraTokens.value(key);
                }
            }
#endif
            emit finished(AccountTaskState::STATE_WORKING, tr("Got "));
            return;
        }
        case Katabasis::Activity::FailedSoft: {
            // NOTE: soft error in the first step means 'offline'
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Microsoft user authentication ended with a network error."));
            return;
        }
        case Katabasis::Activity::FailedGone: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_GONE, tr("Microsoft user authentication failed - user no longer exists."));
            return;
        }
        case Katabasis::Activity::FailedHard: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Microsoft user authentication failed."));
            return;
        }
        default: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Microsoft user authentication completed with an unrecognized result."));
            return;
        }
    }
}
