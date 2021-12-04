#include "AuthStep.h"

AuthStep::AuthStep(AccountData *data) : QObject(nullptr), m_data(data) {
}

AuthStep::~AuthStep() noexcept = default;

