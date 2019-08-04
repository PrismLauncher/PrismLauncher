#include "MinecraftInstance.h"
#include <minecraft/launch/CreateServerResourcePacksFolder.h>
#include <minecraft/launch/ExtractNatives.h>
#include <minecraft/launch/PrintInstanceInfo.h>
#include <settings/Setting.h>
#include "settings/SettingsObject.h"
#include "Env.h"
#include <MMCStrings.h>
#include <pathmatcher/RegexpMatcher.h>
#include <pathmatcher/MultiMatcher.h>
#include <FileSystem.h>
#include <java/JavaVersion.h>

#include "launch/LaunchTask.h"
#include "launch/steps/PostLaunchCommand.h"
#include "launch/steps/Update.h"
#include "launch/steps/PreLaunchCommand.h"
#include "launch/steps/TextPrint.h"
#include "minecraft/launch/LauncherPartLaunch.h"
#include "minecraft/launch/DirectJavaLaunch.h"
#include "minecraft/launch/ModMinecraftJar.h"
#include "minecraft/launch/ClaimAccount.h"
#include "minecraft/launch/ReconstructAssets.h"
#include "java/launch/CheckJava.h"
#include "java/JavaUtils.h"
#include "meta/Index.h"
#include "meta/VersionList.h"

#include "mod/ModFolderModel.h"
#include "WorldList.h"

#include "icons/IIconList.h"

#include <QCoreApplication>
#include "ComponentList.h"
#include "AssetsUtils.h"
#include "MinecraftUpdate.h"
#include "MinecraftLoadAndCheck.h"
#include <minecraft/gameoptions/GameOptions.h>

#define IBUS "@im=ibus"

// all of this because keeping things compatible with deprecated old settings
// if either of the settings {a, b} is true, this also resolves to true
class OrSetting : public Setting
{
    Q_OBJECT
public:
    OrSetting(QString id, std::shared_ptr<Setting> a, std::shared_ptr<Setting> b)
    :Setting({id}, false), m_a(a), m_b(b)
    {
    }
    virtual QVariant get() const
    {
        bool a = m_a->get().toBool();
        bool b = m_b->get().toBool();
        return a || b;
    }
    virtual void reset() {}
    virtual void set(QVariant value) {}
private:
    std::shared_ptr<Setting> m_a;
    std::shared_ptr<Setting> m_b;
};

MinecraftInstance::MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
    : BaseInstance(globalSettings, settings, rootDir)
{
    // Java Settings
    auto javaOverride = m_settings->registerSetting("OverrideJava", false);
    auto locationOverride = m_settings->registerSetting("OverrideJavaLocation", false);
    auto argsOverride = m_settings->registerSetting("OverrideJavaArgs", false);

    // combinations
    auto javaOrLocation = std::make_shared<OrSetting>("JavaOrLocationOverride", javaOverride, locationOverride);
    auto javaOrArgs = std::make_shared<OrSetting>("JavaOrArgsOverride", javaOverride, argsOverride);

    m_settings->registerOverride(globalSettings->getSetting("JavaPath"), javaOrLocation);
    m_settings->registerOverride(globalSettings->getSetting("JvmArgs"), javaOrArgs);

    // special!
    m_settings->registerPassthrough(globalSettings->getSetting("JavaTimestamp"), javaOrLocation);
    m_settings->registerPassthrough(globalSettings->getSetting("JavaVersion"), javaOrLocation);
    m_settings->registerPassthrough(globalSettings->getSetting("JavaArchitecture"), javaOrLocation);

    // Window Size
    auto windowSetting = m_settings->registerSetting("OverrideWindow", false);
    m_settings->registerOverride(globalSettings->getSetting("LaunchMaximized"), windowSetting);
    m_settings->registerOverride(globalSettings->getSetting("MinecraftWinWidth"), windowSetting);
    m_settings->registerOverride(globalSettings->getSetting("MinecraftWinHeight"), windowSetting);

    // Memory
    auto memorySetting = m_settings->registerSetting("OverrideMemory", false);
    m_settings->registerOverride(globalSettings->getSetting("MinMemAlloc"), memorySetting);
    m_settings->registerOverride(globalSettings->getSetting("MaxMemAlloc"), memorySetting);
    m_settings->registerOverride(globalSettings->getSetting("PermGen"), memorySetting);

    // Minecraft launch method
    auto launchMethodOverride = m_settings->registerSetting("OverrideMCLaunchMethod", false);
    m_settings->registerOverride(globalSettings->getSetting("MCLaunchMethod"), launchMethodOverride);

    // DEPRECATED: Read what versions the user configuration thinks should be used
    m_settings->registerSetting({"IntendedVersion", "MinecraftVersion"}, "");
    m_settings->registerSetting("LWJGLVersion", "");
    m_settings->registerSetting("ForgeVersion", "");
    m_settings->registerSetting("LiteloaderVersion", "");

    m_components.reset(new ComponentList(this));
    m_components->setOldConfigVersion("net.minecraft", m_settings->get("IntendedVersion").toString());
    auto setting = m_settings->getSetting("LWJGLVersion");
    m_components->setOldConfigVersion("org.lwjgl", m_settings->get("LWJGLVersion").toString());
    m_components->setOldConfigVersion("net.minecraftforge", m_settings->get("ForgeVersion").toString());
    m_components->setOldConfigVersion("com.mumfrey.liteloader", m_settings->get("LiteloaderVersion").toString());
}

void MinecraftInstance::saveNow()
{
    m_components->saveNow();
}

QString MinecraftInstance::typeName() const
{
    return "Minecraft";
}

std::shared_ptr<ComponentList> MinecraftInstance::getComponentList() const
{
    return m_components;
}

QSet<QString> MinecraftInstance::traits() const
{
    auto components = getComponentList();
    if (!components)
    {
        return {"version-incomplete"};
    }
    auto profile = components->getProfile();
    if (!profile)
    {
        return {"version-incomplete"};
    }
    return profile->getTraits();
}

QString MinecraftInstance::gameRoot() const
{
    QFileInfo mcDir(FS::PathCombine(instanceRoot(), "minecraft"));
    QFileInfo dotMCDir(FS::PathCombine(instanceRoot(), ".minecraft"));

    if (mcDir.exists() && !dotMCDir.exists())
        return mcDir.filePath();
    else
        return dotMCDir.filePath();
}

QString MinecraftInstance::binRoot() const
{
    return FS::PathCombine(gameRoot(), "bin");
}

QString MinecraftInstance::getNativePath() const
{
    QDir natives_dir(FS::PathCombine(instanceRoot(), "natives/"));
    return natives_dir.absolutePath();
}

QString MinecraftInstance::getLocalLibraryPath() const
{
    QDir libraries_dir(FS::PathCombine(instanceRoot(), "libraries/"));
    return libraries_dir.absolutePath();
}

QString MinecraftInstance::jarModsDir() const
{
    QDir jarmods_dir(FS::PathCombine(instanceRoot(), "jarmods/"));
    return jarmods_dir.absolutePath();
}

QString MinecraftInstance::loaderModsDir() const
{
    return FS::PathCombine(gameRoot(), "mods");
}

QString MinecraftInstance::modsCacheLocation() const
{
    return FS::PathCombine(instanceRoot(), "mods.cache");
}

QString MinecraftInstance::coreModsDir() const
{
    return FS::PathCombine(gameRoot(), "coremods");
}

QString MinecraftInstance::resourcePacksDir() const
{
    return FS::PathCombine(gameRoot(), "resourcepacks");
}

QString MinecraftInstance::texturePacksDir() const
{
    return FS::PathCombine(gameRoot(), "texturepacks");
}

QString MinecraftInstance::instanceConfigFolder() const
{
    return FS::PathCombine(gameRoot(), "config");
}

QString MinecraftInstance::libDir() const
{
    return FS::PathCombine(gameRoot(), "lib");
}

QString MinecraftInstance::worldDir() const
{
    return FS::PathCombine(gameRoot(), "saves");
}

QString MinecraftInstance::resourcesDir() const
{
    return FS::PathCombine(gameRoot(), "resources");
}

QDir MinecraftInstance::librariesPath() const
{
    return QDir::current().absoluteFilePath("libraries");
}

QDir MinecraftInstance::jarmodsPath() const
{
    return QDir(jarModsDir());
}

QDir MinecraftInstance::versionsPath() const
{
    return QDir::current().absoluteFilePath("versions");
}

QStringList MinecraftInstance::getClassPath() const
{
    QStringList jars, nativeJars;
    auto javaArchitecture = settings()->get("JavaArchitecture").toString();
    auto profile = m_components->getProfile();
    profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
    return jars;
}

QString MinecraftInstance::getMainClass() const
{
    auto profile = m_components->getProfile();
    return profile->getMainClass();
}

QStringList MinecraftInstance::getNativeJars() const
{
    QStringList jars, nativeJars;
    auto javaArchitecture = settings()->get("JavaArchitecture").toString();
    auto profile = m_components->getProfile();
    profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
    return nativeJars;
}

QStringList MinecraftInstance::extraArguments() const
{
    auto list = BaseInstance::extraArguments();
    auto version = getComponentList();
    if (!version)
        return list;
    auto jarMods = getJarMods();
    if (!jarMods.isEmpty())
    {
        list.append({"-Dfml.ignoreInvalidMinecraftCertificates=true",
                     "-Dfml.ignorePatchDiscrepancies=true"});
    }
    return list;
}

QStringList MinecraftInstance::javaArguments() const
{
    QStringList args;

    // custom args go first. we want to override them if we have our own here.
    args.append(extraArguments());

    // OSX dock icon and name
#ifdef Q_OS_MAC
    args << "-Xdock:icon=icon.png";
    args << QString("-Xdock:name=\"%1\"").arg(windowTitle());
#endif
    auto traits_ = traits();
    // HACK: fix issues on macOS with 1.13 snapshots
    // NOTE: Oracle Java option. if there are alternate jvm implementations, this would be the place to customize this for them
#ifdef Q_OS_MAC
    if(traits_.contains("FirstThreadOnMacOS"))
    {
        args << QString("-XstartOnFirstThread");
    }
#endif

    // HACK: Stupid hack for Intel drivers. See: https://mojang.atlassian.net/browse/MCL-767
#ifdef Q_OS_WIN32
    args << QString("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_"
                    "minecraft.exe.heapdump");
#endif

    int min = settings()->get("MinMemAlloc").toInt();
    int max = settings()->get("MaxMemAlloc").toInt();
    if(min < max)
    {
        args << QString("-Xms%1m").arg(min);
        args << QString("-Xmx%1m").arg(max);
    }
    else
    {
        args << QString("-Xms%1m").arg(max);
        args << QString("-Xmx%1m").arg(min);
    }

    // No PermGen in newer java.
    JavaVersion javaVersion = getJavaVersion();
    if(javaVersion.requiresPermGen())
    {
        auto permgen = settings()->get("PermGen").toInt();
        if (permgen != 64)
        {
            args << QString("-XX:PermSize=%1m").arg(permgen);
        }
    }

    args << "-Duser.language=en";

    return args;
}

QMap<QString, QString> MinecraftInstance::getVariables() const
{
    QMap<QString, QString> out;
    out.insert("INST_NAME", name());
    out.insert("INST_ID", id());
    out.insert("INST_DIR", QDir(instanceRoot()).absolutePath());
    out.insert("INST_MC_DIR", QDir(gameRoot()).absolutePath());
    out.insert("INST_JAVA", settings()->get("JavaPath").toString());
    out.insert("INST_JAVA_ARGS", javaArguments().join(' '));
    return out;
}

QProcessEnvironment MinecraftInstance::createEnvironment()
{
    // prepare the process environment
    QProcessEnvironment env = CleanEnviroment();

    // export some infos
    auto variables = getVariables();
    for (auto it = variables.begin(); it != variables.end(); ++it)
    {
        env.insert(it.key(), it.value());
    }
    return env;
}

static QString replaceTokensIn(QString text, QMap<QString, QString> with)
{
    QString result;
    QRegExp token_regexp("\\$\\{(.+)\\}");
    token_regexp.setMinimal(true);
    QStringList list;
    int tail = 0;
    int head = 0;
    while ((head = token_regexp.indexIn(text, head)) != -1)
    {
        result.append(text.mid(tail, head - tail));
        QString key = token_regexp.cap(1);
        auto iter = with.find(key);
        if (iter != with.end())
        {
            result.append(*iter);
        }
        head += token_regexp.matchedLength();
        tail = head;
    }
    result.append(text.mid(tail));
    return result;
}

QStringList MinecraftInstance::processMinecraftArgs(AuthSessionPtr session) const
{
    auto profile = m_components->getProfile();
    QString args_pattern = profile->getMinecraftArguments();
    for (auto tweaker : profile->getTweakers())
    {
        args_pattern += " --tweakClass " + tweaker;
    }

    QMap<QString, QString> token_mapping;
    // yggdrasil!
    if(session)
    {
        token_mapping["auth_username"] = session->username;
        token_mapping["auth_session"] = session->session;
        token_mapping["auth_access_token"] = session->access_token;
        token_mapping["auth_player_name"] = session->player_name;
        token_mapping["auth_uuid"] = session->uuid;
        token_mapping["user_properties"] = session->serializeUserProperties();
        token_mapping["user_type"] = session->user_type;
    }

    // blatant self-promotion.
    token_mapping["profile_name"] = token_mapping["version_name"] = "MultiMC5";

    token_mapping["version_type"] = profile->getMinecraftVersionType();

    QString absRootDir = QDir(gameRoot()).absolutePath();
    token_mapping["game_directory"] = absRootDir;
    QString absAssetsDir = QDir("assets/").absolutePath();
    auto assets = profile->getMinecraftAssets();
    token_mapping["game_assets"] = AssetsUtils::getAssetsDir(assets->id, resourcesDir()).absolutePath();

    // 1.7.3+ assets tokens
    token_mapping["assets_root"] = absAssetsDir;
    token_mapping["assets_index_name"] = assets->id;

    QStringList parts = args_pattern.split(' ', QString::SkipEmptyParts);
    for (int i = 0; i < parts.length(); i++)
    {
        parts[i] = replaceTokensIn(parts[i], token_mapping);
    }
    return parts;
}

QString MinecraftInstance::createLaunchScript(AuthSessionPtr session)
{
    QString launchScript;

    if (!m_components)
        return QString();
    auto profile = m_components->getProfile();
    if(!profile)
        return QString();

    auto mainClass = getMainClass();
    if (!mainClass.isEmpty())
    {
        launchScript += "mainClass " + mainClass + "\n";
    }
    auto appletClass = profile->getAppletClass();
    if (!appletClass.isEmpty())
    {
        launchScript += "appletClass " + appletClass + "\n";
    }

    // generic minecraft params
    for (auto param : processMinecraftArgs(session))
    {
        launchScript += "param " + param + "\n";
    }

    // window size, title and state, legacy
    {
        QString windowParams;
        if (settings()->get("LaunchMaximized").toBool())
            windowParams = "max";
        else
            windowParams = QString("%1x%2")
                               .arg(settings()->get("MinecraftWinWidth").toInt())
                               .arg(settings()->get("MinecraftWinHeight").toInt());
        launchScript += "windowTitle " + windowTitle() + "\n";
        launchScript += "windowParams " + windowParams + "\n";
    }

    // legacy auth
    if(session)
    {
        launchScript += "userName " + session->player_name + "\n";
        launchScript += "sessionId " + session->session + "\n";
    }

    // libraries and class path.
    {
        QStringList jars, nativeJars;
        auto javaArchitecture = settings()->get("JavaArchitecture").toString();
        profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
        for(auto file: jars)
        {
            launchScript += "cp " + file + "\n";
        }
        for(auto file: nativeJars)
        {
            launchScript += "ext " + file + "\n";
        }
        launchScript += "natives " + getNativePath() + "\n";
    }

    for (auto trait : profile->getTraits())
    {
        launchScript += "traits " + trait + "\n";
    }
    launchScript += "launcher onesix\n";
    // qDebug() << "Generated launch script:" << launchScript;
    return launchScript;
}

QStringList MinecraftInstance::verboseDescription(AuthSessionPtr session)
{
    QStringList out;
    out << "Main Class:" << "  " + getMainClass() << "";
    out << "Native path:" << "  " + getNativePath() << "";

    auto profile = m_components->getProfile();

    auto alltraits = traits();
    if(alltraits.size())
    {
        out << "Traits:";
        for (auto trait : alltraits)
        {
            out << "traits " + trait;
        }
        out << "";
    }

    // libraries and class path.
    {
        out << "Libraries:";
        QStringList jars, nativeJars;
        auto javaArchitecture = settings()->get("JavaArchitecture").toString();
        profile->getLibraryFiles(javaArchitecture, jars, nativeJars, getLocalLibraryPath(), binRoot());
        auto printLibFile = [&](const QString & path)
        {
            QFileInfo info(path);
            if(info.exists())
            {
                out << "  " + path;
            }
            else
            {
                out << "  " + path + " (missing)";
            }
        };
        for(auto file: jars)
        {
            printLibFile(file);
        }
        out << "";
        out << "Native libraries:";
        for(auto file: nativeJars)
        {
            printLibFile(file);
        }
        out << "";
    }

    if(loaderModList()->size())
    {
        out << "Mods:";
        for(auto & mod: loaderModList()->allMods())
        {
            if(!mod.enabled())
                continue;
            if(mod.type() == Mod::MOD_FOLDER)
                continue;
            // TODO: proper implementation would need to descend into folders.

            out << "  " + mod.filename().completeBaseName();
        }
        out << "";
    }

    if(coreModList()->size())
    {
        out << "Core Mods:";
        for(auto & coremod: coreModList()->allMods())
        {
            if(!coremod.enabled())
                continue;
            if(coremod.type() == Mod::MOD_FOLDER)
                continue;
            // TODO: proper implementation would need to descend into folders.

            out << "  " + coremod.filename().completeBaseName();
        }
        out << "";
    }

    auto & jarMods = profile->getJarMods();
    if(jarMods.size())
    {
        out << "Jar Mods:";
        for(auto & jarmod: jarMods)
        {
            auto displayname = jarmod->displayName(currentSystem);
            auto realname = jarmod->filename(currentSystem);
            if(displayname != realname)
            {
                out << "  " + displayname + " (" + realname + ")";
            }
            else
            {
                out << "  " + realname;
            }
        }
        out << "";
    }

    auto params = processMinecraftArgs(nullptr);
    out << "Params:";
    out << "  " + params.join(' ');
    out << "";

    QString windowParams;
    if (settings()->get("LaunchMaximized").toBool())
    {
        out << "Window size: max (if available)";
    }
    else
    {
        auto width = settings()->get("MinecraftWinWidth").toInt();
        auto height = settings()->get("MinecraftWinHeight").toInt();
        out << "Window size: " + QString::number(width) + " x " + QString::number(height);
    }
    out << "";
    return out;
}

QMap<QString, QString> MinecraftInstance::createCensorFilterFromSession(AuthSessionPtr session)
{
    if(!session)
    {
        return QMap<QString, QString>();
    }
    auto & sessionRef = *session.get();
    QMap<QString, QString> filter;
    auto addToFilter = [&filter](QString key, QString value)
    {
        if(key.trimmed().size())
        {
            filter[key] = value;
        }
    };
    if (sessionRef.session != "-")
    {
        addToFilter(sessionRef.session, tr("<SESSION ID>"));
    }
    addToFilter(sessionRef.access_token, tr("<ACCESS TOKEN>"));
    addToFilter(sessionRef.client_token, tr("<CLIENT TOKEN>"));
    addToFilter(sessionRef.uuid, tr("<PROFILE ID>"));

    auto i = sessionRef.u.properties.begin();
    while (i != sessionRef.u.properties.end())
    {
        if(i.value().length() <= 3) {
            ++i;
            continue;
        }
        addToFilter(i.value(), "<" + i.key().toUpper() + ">");
        ++i;
    }
    return filter;
}

MessageLevel::Enum MinecraftInstance::guessLevel(const QString &line, MessageLevel::Enum level)
{
    QRegularExpression re("\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
    auto match = re.match(line);
    if(match.hasMatch())
    {
        // New style logs from log4j
        QString timestamp = match.captured("timestamp");
        QString levelStr = match.captured("level");
        if(levelStr == "INFO")
            level = MessageLevel::Message;
        if(levelStr == "WARN")
            level = MessageLevel::Warning;
        if(levelStr == "ERROR")
            level = MessageLevel::Error;
        if(levelStr == "FATAL")
            level = MessageLevel::Fatal;
        if(levelStr == "TRACE" || levelStr == "DEBUG")
            level = MessageLevel::Debug;
    }
    else
    {
        // Old style forge logs
        if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") ||
            line.contains("[FINER]") || line.contains("[FINEST]"))
            level = MessageLevel::Message;
        if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
            level = MessageLevel::Error;
        if (line.contains("[WARNING]"))
            level = MessageLevel::Warning;
        if (line.contains("[DEBUG]"))
            level = MessageLevel::Debug;
    }
    if (line.contains("overwriting existing"))
        return MessageLevel::Fatal;
    //NOTE: this diverges from the real regexp. no unicode, the first section is + instead of *
    static const QString javaSymbol = "([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$][a-zA-Z\\d_$]*";
    if (line.contains("Exception in thread")
        || line.contains(QRegularExpression("\\s+at " + javaSymbol))
        || line.contains(QRegularExpression("Caused by: " + javaSymbol))
        || line.contains(QRegularExpression("([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$]?[a-zA-Z\\d_$]*(Exception|Error|Throwable)"))
        || line.contains(QRegularExpression("... \\d+ more$"))
        )
        return MessageLevel::Error;
    return level;
}

IPathMatcher::Ptr MinecraftInstance::getLogFileMatcher()
{
    auto combined = std::make_shared<MultiMatcher>();
    combined->add(std::make_shared<RegexpMatcher>(".*\\.log(\\.[0-9]*)?(\\.gz)?$"));
    combined->add(std::make_shared<RegexpMatcher>("crash-.*\\.txt"));
    combined->add(std::make_shared<RegexpMatcher>("IDMap dump.*\\.txt$"));
    combined->add(std::make_shared<RegexpMatcher>("ModLoader\\.txt(\\..*)?$"));
    return combined;
}

QString MinecraftInstance::getLogFileRoot()
{
    return gameRoot();
}

QString MinecraftInstance::prettifyTimeDuration(int64_t duration)
{
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    int days = (int) (duration / 24);
    if((hours == 0)&&(days == 0))
    {
        return tr("%1m %2s").arg(minutes).arg(seconds);
    }
    if (days == 0)
    {
        return tr("%1h %2m").arg(hours).arg(minutes);
    }
    return tr("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
}

QString MinecraftInstance::getStatusbarDescription()
{
    QStringList traits;
    if (hasVersionBroken())
    {
        traits.append(tr("broken"));
    }

    QString description;
    description.append(tr("Minecraft %1 (%2)").arg(m_components->getComponentVersion("net.minecraft")).arg(typeName()));
    if(totalTimePlayed() > 0)
    {
        description.append(tr(", played for %1").arg(prettifyTimeDuration(totalTimePlayed())));
    }
    if(hasCrashed())
    {
        description.append(tr(", has crashed."));
    }
    return description;
}

shared_qobject_ptr<Task> MinecraftInstance::createUpdateTask(Net::Mode mode)
{
    switch (mode)
    {
        case Net::Mode::Offline:
        {
            return shared_qobject_ptr<Task>(new MinecraftLoadAndCheck(this));
        }
        case Net::Mode::Online:
        {
            return shared_qobject_ptr<Task>(new MinecraftUpdate(this));
        }
    }
    return nullptr;
}

shared_qobject_ptr<LaunchTask> MinecraftInstance::createLaunchTask(AuthSessionPtr session)
{
    // FIXME: get rid of shared_from_this ...
    auto process = LaunchTask::create(std::dynamic_pointer_cast<MinecraftInstance>(shared_from_this()));
    auto pptr = process.get();

    ENV.icons()->saveIcon(iconKey(), FS::PathCombine(gameRoot(), "icon.png"), "PNG");

    // print a header
    {
        process->appendStep(new TextPrint(pptr, "Minecraft folder is:\n" + gameRoot() + "\n\n", MessageLevel::MultiMC));
    }

    // check java
    {
        process->appendStep(new CheckJava(pptr));
    }

    // check launch method
    QStringList validMethods = {"LauncherPart", "DirectJava"};
    QString method = launchMethod();
    if(!validMethods.contains(method))
    {
        process->appendStep(new TextPrint(pptr, "Selected launch method \"" + method + "\" is not valid.\n", MessageLevel::Fatal));
        return process;
    }

    // run pre-launch command if that's needed
    if(getPreLaunchCommand().size())
    {
        auto step = new PreLaunchCommand(pptr);
        step->setWorkingDirectory(gameRoot());
        process->appendStep(step);
    }

    // if we aren't in offline mode,.
    if(session->status != AuthSession::PlayableOffline)
    {
        process->appendStep(new ClaimAccount(pptr, session));
        process->appendStep(new Update(pptr, Net::Mode::Online));
    }
    else
    {
        process->appendStep(new Update(pptr, Net::Mode::Offline));
    }

    // if there are any jar mods
    {
        process->appendStep(new ModMinecraftJar(pptr));
    }

    // print some instance info here...
    {
        process->appendStep(new PrintInstanceInfo(pptr, session));
    }

    // create the server-resource-packs folder (workaround for Minecraft bug MCL-3732)
    {
        process->appendStep(new CreateServerResourcePacksFolder(pptr));
    }

    // extract native jars if needed
    {
        process->appendStep(new ExtractNatives(pptr));
    }

    // reconstruct assets if needed
    {
        process->appendStep(new ReconstructAssets(pptr));
    }

    {
        // actually launch the game
        auto method = launchMethod();
        if(method == "LauncherPart")
        {
            auto step = new LauncherPartLaunch(pptr);
            step->setWorkingDirectory(gameRoot());
            step->setAuthSession(session);
            process->appendStep(step);
        }
        else if (method == "DirectJava")
        {
            auto step = new DirectJavaLaunch(pptr);
            step->setWorkingDirectory(gameRoot());
            step->setAuthSession(session);
            process->appendStep(step);
        }
    }

    // run post-exit command if that's needed
    if(getPostExitCommand().size())
    {
        auto step = new PostLaunchCommand(pptr);
        step->setWorkingDirectory(gameRoot());
        process->appendStep(step);
    }
    if (session)
    {
        process->setCensorFilter(createCensorFilterFromSession(session));
    }
    m_launchProcess = process;
    emit launchTaskChanged(m_launchProcess);
    return m_launchProcess;
}

QString MinecraftInstance::launchMethod()
{
    return m_settings->get("MCLaunchMethod").toString();
}

JavaVersion MinecraftInstance::getJavaVersion() const
{
    return JavaVersion(settings()->get("JavaVersion").toString());
}

std::shared_ptr<ModFolderModel> MinecraftInstance::loaderModList() const
{
    if (!m_loader_mod_list)
    {
        m_loader_mod_list.reset(new ModFolderModel(loaderModsDir()));
        m_loader_mod_list->disableInteraction(isRunning());
        connect(this, &BaseInstance::runningStatusChanged, m_loader_mod_list.get(), &ModFolderModel::disableInteraction);
    }
    return m_loader_mod_list;
}

std::shared_ptr<ModFolderModel> MinecraftInstance::coreModList() const
{
    if (!m_core_mod_list)
    {
        m_core_mod_list.reset(new ModFolderModel(coreModsDir()));
        m_core_mod_list->disableInteraction(isRunning());
        connect(this, &BaseInstance::runningStatusChanged, m_core_mod_list.get(), &ModFolderModel::disableInteraction);
    }
    return m_core_mod_list;
}

std::shared_ptr<ModFolderModel> MinecraftInstance::resourcePackList() const
{
    if (!m_resource_pack_list)
    {
        m_resource_pack_list.reset(new ModFolderModel(resourcePacksDir()));
        m_resource_pack_list->disableInteraction(isRunning());
        connect(this, &BaseInstance::runningStatusChanged, m_resource_pack_list.get(), &ModFolderModel::disableInteraction);
    }
    return m_resource_pack_list;
}

std::shared_ptr<ModFolderModel> MinecraftInstance::texturePackList() const
{
    if (!m_texture_pack_list)
    {
        m_texture_pack_list.reset(new ModFolderModel(texturePacksDir()));
        m_texture_pack_list->disableInteraction(isRunning());
        connect(this, &BaseInstance::runningStatusChanged, m_texture_pack_list.get(), &ModFolderModel::disableInteraction);
    }
    return m_texture_pack_list;
}

std::shared_ptr<WorldList> MinecraftInstance::worldList() const
{
    if (!m_world_list)
    {
        m_world_list.reset(new WorldList(worldDir()));
    }
    return m_world_list;
}

std::shared_ptr<GameOptions> MinecraftInstance::gameOptionsModel() const
{
    if (!m_game_options)
    {
        m_game_options.reset(new GameOptions(FS::PathCombine(gameRoot(), "options.txt")));
    }
    return m_game_options;
}

QList< Mod > MinecraftInstance::getJarMods() const
{
    auto profile = m_components->getProfile();
    QList<Mod> mods;
    for (auto jarmod : profile->getJarMods())
    {
        QStringList jar, temp1, temp2, temp3;
        jarmod->getApplicableFiles(currentSystem, jar, temp1, temp2, temp3, jarmodsPath().absolutePath());
        // QString filePath = jarmodsPath().absoluteFilePath(jarmod->filename(currentSystem));
        mods.push_back(Mod(QFileInfo(jar[0])));
    }
    return mods;
}


#include "MinecraftInstance.moc"
