#include "MigrationEligibilityStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

MigrationEligibilityStep::MigrationEligibilityStep(AccountData* data) : AuthStep(data) {

}

MigrationEligibilityStep::~MigrationEligibilityStep() noexcept = default;

QString MigrationEligibilityStep::describe() {
    return tr("Checking for migration eligibility.");
}

void MigrationEligibilityStep::perform() {
    auto url = QUrl("https://api.minecraftservices.com/rollout/v1/msamigration");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data->yggdrasilToken.token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &MigrationEligibilityStep::onRequestDone);
    requestor->get(request);
}

void MigrationEligibilityStep::rehydrate() {
    // NOOP, for now. We only save bools and there's nothing to check.
}

void MigrationEligibilityStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error == QNetworkReply::NoError) {
        Parsers::parseRolloutResponse(data, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data->canMigrateToMSA);
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("Got migration flags"));
}
