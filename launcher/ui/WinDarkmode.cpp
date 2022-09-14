#include <QWidget>

#include "WinDarkmode.h"

namespace WinDarkmode {

/* See https://github.com/statiolake/neovim-qt/commit/da8eaba7f0e38b6b51f3bacd02a8cc2d1f7a34d8 */
void setDarkWinTitlebar(WId winid, bool darkmode)
{
    HWND hwnd = reinterpret_cast<HWND>(winid);
    BOOL dark = (BOOL) darkmode;

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    fnAllowDarkModeForWindow AllowDarkModeForWindow
        = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
    fnSetPreferredAppMode SetPreferredAppMode
        = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    fnSetWindowCompositionAttribute SetWindowCompositionAttribute
        = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));

    SetPreferredAppMode(AllowDark);
    AllowDarkModeForWindow(hwnd, dark);
    WINDOWCOMPOSITIONATTRIBDATA data = {
        WCA_USEDARKMODECOLORS,
        &dark,
        sizeof(dark)
    };
    SetWindowCompositionAttribute(hwnd, &data);
}

}
