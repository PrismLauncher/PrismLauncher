// Check if this file exists - if not, we'll create a minimal version with offline mode startup check
// This is a helper header to enable startup offline account greeting

#pragma once

#include <QString>
#include <QObject>

class Application;

// Forward declaration for startup check
bool shouldShowOfflineModeGreeting(Application* app);
void showOfflineModeGreeting(class QWidget* parent, Application* app);
