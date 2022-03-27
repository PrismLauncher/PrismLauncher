#include <QFileInfo>

namespace Flatpak
{
   bool IsFlatpak()
   {
     #ifdef Q_OS_LINUX
     QFileInfo check_file("/.flatpak-info");
     return check_file.exists();
     #else
     return false;
     #endif
   } 
}
