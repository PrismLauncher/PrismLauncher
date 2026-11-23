#pragma once
#include <QList>
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AccountData.h"

class NetworkContext;

/**
 * Enum for describing the state of the current task.
 * Used by the getStateMessage function to determine what the status message should be.
 */
enum class AccountTaskState {
    STATE_CREATED,
    STATE_WORKING,
    STATE_SUCCEEDED,
    STATE_DISABLED,     //!< MSA Client ID has changed. Tell user to reloginn
    STATE_FAILED_SOFT,  //!< soft failure. authentication went through partially
    STATE_FAILED_HARD,  //!< hard failure. main tokens are invalid
    STATE_FAILED_GONE,  //!< hard failure. main tokens are invalid, and the account no longer exists
    STATE_OFFLINE       //!< soft failure. authentication failed in the first step in a 'soft' way
};

class AuthStep : public QObject {
    Q_OBJECT

   public:
    using Ptr = shared_qobject_ptr<AuthStep>;

    AuthStep(AccountData* data, NetworkContext* network) : QObject(nullptr), m_data(data), m_network(network) {};
    virtual ~AuthStep() noexcept = default;

    virtual QString describe() = 0;

   public slots:
    virtual void perform() = 0;
    virtual void abort() {}

   signals:
    void finished(AccountTaskState resultingState, QString message);

   protected:
    AccountData* m_data;
    NetworkContext* m_network;
};
