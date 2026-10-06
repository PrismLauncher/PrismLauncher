// OFFLINE MODE JAVA VERIFICATION
// Make Java check optional for offline mode
//
// Modify verification logic:
// - If account is offline: make Java check non-blocking warning
// - Allow user to override and continue without Java installation check
// - For online accounts: enforce Java requirement (existing behavior)

#include "VerifyJavaInstall.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "launch/LaunchTask.h"

void VerifyJavaInstall::executeTask()
{
    auto instance = m_parent->instance();
    auto account = m_parent->auth();

    // Check Java installation
    auto javaPath = instance->settings()->get("JavaPath").toString();
    
    if (!QFileInfo(javaPath).exists()) {
        // For offline mode, make Java check optional
        if (account && account->accountType() == AccountType::Offline) {
            emit logLine(
                tr("Warning: Java not found, but offline mode allows continuation."),
                MessageLevel::Warning);
            emitSucceeded();  // Don't block offline mode
            return;
        }
        
        // For online mode, fail if Java not found
        emitFailed(tr("Java installation not found. Please install Java or configure the correct path."));
        return;
    }

    emitSucceeded();
}
