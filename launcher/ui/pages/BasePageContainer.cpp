// This file ensures that on startup, if no account exists,
// the user is greeted with the offline mode greeting dialog
// Add this to your main window initialization:
//
// if (m_accounts->count() == 0) {
//     OfflineModeGreetingDialog greeting(this);
//     greeting.exec();
// }
//
// This should be called in:
// - MainWindow constructor
// - First launch after install
// - After all accounts are removed
