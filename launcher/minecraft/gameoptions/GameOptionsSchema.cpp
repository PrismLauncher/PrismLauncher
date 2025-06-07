#include "GameOptionsSchema.h"
#include <QPair>
#include <QVariantList>

QVariantList makeOptions(QList<QPair<QString, QVariant>> data)
{
    QVariantList opt;
    for (auto v : data)
        opt << QVariant::fromValue((QVariantList({ v.first, v.second })));
    return opt;
}
const std::vector<GameOption> globalDelegateMap = {
    { "ao", true, QObject::tr("Smooth lighting") },
    { "biomeBlendRadius", 2, QObject::tr("Radius for which biome blending should happen"), 0, 7 },
    { "enableVsync", true, QObject::tr("Whether v-sync (vertical synchronization) is enabled") },
    { "entityDistanceScaling", 1.0, QObject::tr("The maximum distance from the player that entities render"), 0.5, 5.0 },
    { "entityShadows", true, QObject::tr("Whether to display entity shadows ") },
    { "forceUnicodeFont", false, QObject::tr("Whether Unicode font should be used") },
    { "japaneseGlyphVariants", false,
      QObject::tr("Uses Japanese variants of CJK (Chinese, Japanese, and Korean) characters in the default font") },
    { "fov", 0.0,
      QObject::tr("How large the field of view is (floating-point). The in-game value is counted in degrees, however, the options.txt "
                  "isn't. The value is converted into degrees with the following formula: degrees = 40 * value + 70"),
      -1, 1 },
    { "fovEffectScale", 1.0,
      QObject::tr(" 	FOV Effects (how much the field of view changes when sprinting, having Speed or Slowness etc.)"), 0.0, 1.0 },
    { "darknessEffectScale", 1.0, QObject::tr("Darkness Pulsing (how much the Darkness effect pulses)"), 0.0, 1.0 },
    { "glintSpeed", 0.5, QObject::tr("The speed of visual glint on enchanted items "), 0.0, 1.0 },
    { "glintStrength", 0.75, QObject::tr("The strength of visual glint on enchanted items"), 0.0, 1.0 },
    {
        "prioritizeChunkUpdates",
        0,
        QObject::tr("Chunk section update strategy"),
        0,
        2,
        makeOptions({ { "Threaded", 0 }, { "Semi Blocking", 1 }, { "Fully Blocking", 2 } }),
    },
};
