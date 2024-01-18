#include "FusionTheme.h"
#include "ThemeManager.h"

#include <QQuickStyle>

const QString FusionTheme::USE_FUSION_QML_GLOBAL_THEME = "USE_FUSION";

void FusionTheme::apply(bool initial)
{
    changed_qqc_theme = (QQuickStyle::name() != QLatin1String("Fusion"));
    ThemeManager::writeGlobalQMLTheme(USE_FUSION_QML_GLOBAL_THEME);

    ITheme::apply(initial);
}

QString FusionTheme::qtTheme()
{
    return "Fusion";
}
