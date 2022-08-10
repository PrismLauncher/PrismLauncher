#pragma once
#include "BaseInstance.h"
#include <java/JavaVersion.h>
#include "minecraft/mod/Mod.h"
#include <QProcess>
#include <QDir>
#include "minecraft/launch/MinecraftServerTarget.h"

class ModFolderModel;
class ResourceFolderModel;
class ResourcePackFolderModel;
class ShaderPackFolderModel;
class TexturePackFolderModel;
class WorldList;
class GameOptions;
class LaunchStep;
class PackProfile;

class MinecraftInstance: public BaseInstance
{
    Q_OBJECT
public:
    MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
    virtual ~MinecraftInstance() {};
    virtual void saveNow() override;

    void loadSpecificSettings() override;

    // FIXME: remove
    QString typeName() const override;
    // FIXME: remove
    QSet<QString> traits() const override;

    bool canEdit() const override
    {
        return true;
    }

    bool canExport() const override
    {
        return true;
    }

    ////// Directories and files //////
    QString jarModsDir() const;
    QString resourcePacksDir() const;
    QString texturePacksDir() const;
    QString shaderPacksDir() const;
    QString modsRoot() const override;
    QString coreModsDir() const;
    QString modsCacheLocation() const;
    QString libDir() const;
    QString worldDir() const;
    QString resourcesDir() const;
    QDir jarmodsPath() const;
    QDir librariesPath() const;
    QDir versionsPath() const;
    QString instanceConfigFolder() const override;

    // Path to the instance's minecraft directory.
    QString gameRoot() const override;

    // Path to the instance's minecraft bin directory.
    QString binRoot() const;

    // where to put the natives during/before launch
    QString getNativePath() const;

    // where the instance-local libraries should be
    QString getLocalLibraryPath() const;


    //////  Profile management //////
    std::shared_ptr<PackProfile> getPackProfile() const;

    //////  Mod Lists  //////
    std::shared_ptr<ModFolderModel> loaderModList() const;
    std::shared_ptr<ModFolderModel> coreModList() const;
    std::shared_ptr<ResourcePackFolderModel> resourcePackList() const;
    std::shared_ptr<TexturePackFolderModel> texturePackList() const;
    std::shared_ptr<ShaderPackFolderModel> shaderPackList() const;
    std::shared_ptr<WorldList> worldList() const;
    std::shared_ptr<GameOptions> gameOptionsModel() const;

    //////  Launch stuff //////
    Task::Ptr createUpdateTask(Net::Mode mode) override;
    shared_qobject_ptr<LaunchTask> createLaunchTask(AuthSessionPtr account, MinecraftServerTargetPtr serverToJoin) override;
    QStringList extraArguments() override;
    QStringList verboseDescription(AuthSessionPtr session, MinecraftServerTargetPtr serverToJoin) override;
    QList<Mod*> getJarMods() const;
    QString createLaunchScript(AuthSessionPtr session, MinecraftServerTargetPtr serverToJoin);
    /// get arguments passed to java
    QStringList javaArguments();

    /// get variables for launch command variable substitution/environment
    QMap<QString, QString> getVariables() override;

    /// create an environment for launching processes
    QProcessEnvironment createEnvironment() override;
    QProcessEnvironment createLaunchEnvironment() override;

    /// guess log level from a line of minecraft log
    MessageLevel::Enum guessLevel(const QString &line, MessageLevel::Enum level) override;

    IPathMatcher::Ptr getLogFileMatcher() override;

    QString getLogFileRoot() override;

    QString getStatusbarDescription() override;

    // FIXME: remove
    virtual QStringList getClassPath();
    // FIXME: remove
    virtual QStringList getNativeJars();
    // FIXME: remove
    virtual QString getMainClass() const;

    // FIXME: remove
    virtual QStringList processMinecraftArgs(AuthSessionPtr account, MinecraftServerTargetPtr serverToJoin) const;

    virtual JavaVersion getJavaVersion();

protected:
    QMap<QString, QString> createCensorFilterFromSession(AuthSessionPtr session);
    QStringList validLaunchMethods();
    QString launchMethod();

protected: // data
    std::shared_ptr<PackProfile> m_components;
    mutable std::shared_ptr<ModFolderModel> m_loader_mod_list;
    mutable std::shared_ptr<ModFolderModel> m_core_mod_list;
    mutable std::shared_ptr<ResourcePackFolderModel> m_resource_pack_list;
    mutable std::shared_ptr<ShaderPackFolderModel> m_shader_pack_list;
    mutable std::shared_ptr<TexturePackFolderModel> m_texture_pack_list;
    mutable std::shared_ptr<WorldList> m_world_list;
    mutable std::shared_ptr<GameOptions> m_game_options;
};

typedef std::shared_ptr<MinecraftInstance> MinecraftInstancePtr;
