#include "VersionFilterData.h"
#include "ParseUtils.h"

VersionFilterData g_VersionFilterData = VersionFilterData();

VersionFilterData::VersionFilterData()
{
    // 1.3.*
    auto libs13 = QList<FMLlib>{ { "argo-2.25.jar", "bb672829fde76cb163004752b86b0484bd0a7f4b" },
                                 { "guava-12.0.1.jar", "b8e78b9af7bf45900e14c6f958486b6ca682195f" },
                                 { "asm-all-4.0.jar", "98308890597acb64047f7e896638e0d98753ae82" } };

    fmlLibsMapping["1.3.2"] = libs13;

    // 1.4.*
    auto libs14 = QList<FMLlib>{ { "argo-2.25.jar", "bb672829fde76cb163004752b86b0484bd0a7f4b" },
                                 { "guava-12.0.1.jar", "b8e78b9af7bf45900e14c6f958486b6ca682195f" },
                                 { "asm-all-4.0.jar", "98308890597acb64047f7e896638e0d98753ae82" },
                                 { "bcprov-jdk15on-147.jar", "b6f5d9926b0afbde9f4dbe3db88c5247be7794bb" } };

    fmlLibsMapping["1.4"] = libs14;
    fmlLibsMapping["1.4.1"] = libs14;
    fmlLibsMapping["1.4.2"] = libs14;
    fmlLibsMapping["1.4.3"] = libs14;
    fmlLibsMapping["1.4.4"] = libs14;
    fmlLibsMapping["1.4.5"] = libs14;
    fmlLibsMapping["1.4.6"] = libs14;
    fmlLibsMapping["1.4.7"] = libs14;

    // 1.5
    fmlLibsMapping["1.5"] = QList<FMLlib>{ { "argo-small-3.2.jar", "58912ea2858d168c50781f956fa5b59f0f7c6b51" },
                                           { "guava-14.0-rc3.jar", "931ae21fa8014c3ce686aaa621eae565fefb1a6a" },
                                           { "asm-all-4.1.jar", "054986e962b88d8660ae4566475658469595ef58" },
                                           { "bcprov-jdk15on-148.jar", "960dea7c9181ba0b17e8bab0c06a43f0a5f04e65" },
                                           { "deobfuscation_data_1.5.zip", "5f7c142d53776f16304c0bbe10542014abad6af8" },
                                           { "scala-library.jar", "458d046151ad179c85429ed7420ffb1eaf6ddf85" } };

    // 1.5.1
    fmlLibsMapping["1.5.1"] = QList<FMLlib>{ { "argo-small-3.2.jar", "58912ea2858d168c50781f956fa5b59f0f7c6b51" },
                                             { "guava-14.0-rc3.jar", "931ae21fa8014c3ce686aaa621eae565fefb1a6a" },
                                             { "asm-all-4.1.jar", "054986e962b88d8660ae4566475658469595ef58" },
                                             { "bcprov-jdk15on-148.jar", "960dea7c9181ba0b17e8bab0c06a43f0a5f04e65" },
                                             { "deobfuscation_data_1.5.1.zip", "22e221a0d89516c1f721d6cab056a7e37471d0a6" },
                                             { "scala-library.jar", "458d046151ad179c85429ed7420ffb1eaf6ddf85" } };

    // 1.5.2
    fmlLibsMapping["1.5.2"] = QList<FMLlib>{ { "argo-small-3.2.jar", "58912ea2858d168c50781f956fa5b59f0f7c6b51" },
                                             { "guava-14.0-rc3.jar", "931ae21fa8014c3ce686aaa621eae565fefb1a6a" },
                                             { "asm-all-4.1.jar", "054986e962b88d8660ae4566475658469595ef58" },
                                             { "bcprov-jdk15on-148.jar", "960dea7c9181ba0b17e8bab0c06a43f0a5f04e65" },
                                             { "deobfuscation_data_1.5.2.zip", "446e55cd986582c70fcf12cb27bc00114c5adfd9" },
                                             { "scala-library.jar", "458d046151ad179c85429ed7420ffb1eaf6ddf85" } };

    // don't use installers for those.
    forgeInstallerBlacklist = QSet<QString>({ "1.5.2" });

    // FIXME: remove, used for deciding when core mods should display
    legacyCutoffDate = timeFromS3Time("2013-06-25T15:08:56+02:00");
    lwjglWhitelist = QSet<QString>{ "net.java.jinput:jinput", "net.java.jinput:jinput-platform", "net.java.jutils:jutils",
                                    "org.lwjgl.lwjgl:lwjgl",  "org.lwjgl.lwjgl:lwjgl_util",      "org.lwjgl.lwjgl:lwjgl-platform" };

    java8BeginsDate = timeFromS3Time("2017-03-30T09:32:19+00:00");
    java16BeginsDate = timeFromS3Time("2021-05-12T11:19:15+00:00");
    java17BeginsDate = timeFromS3Time("2021-11-16T17:04:48+00:00");
}
