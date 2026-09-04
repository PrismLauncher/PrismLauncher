#pragma once

#include <QObject>
#include "minecraft/MinecraftInstance.h"

class SettingsObject;
class QProcess;

class BaseExternalTool : public QObject {
    Q_OBJECT
   public:
    explicit BaseExternalTool(SettingsObject* settings, MinecraftInstance* instance, QObject* parent = nullptr);
    ~BaseExternalTool() override;

   protected:
    MinecraftInstance* m_instance;
    SettingsObject* m_globalSettings;
};

class BaseDetachedTool : public BaseExternalTool {
    Q_OBJECT
   public:
    explicit BaseDetachedTool(SettingsObject* settings, MinecraftInstance* instance, QObject* parent = nullptr);

   public slots:
    void run();

   protected:
    virtual void runImpl() = 0;
};

class BaseExternalToolFactory {
   public:
    virtual ~BaseExternalToolFactory();

    virtual QString name() const = 0;

    virtual void registerSettings(SettingsObject* settings) = 0;

    virtual BaseExternalTool* createTool(MinecraftInstance* instance, QObject* parent = nullptr) = 0;

    virtual bool check(QString* error) = 0;
    virtual bool check(const QString& path, QString* error) = 0;

   protected:
    SettingsObject* m_globalSettings = nullptr;
};

class BaseDetachedToolFactory : public BaseExternalToolFactory {
   public:
    virtual BaseDetachedTool* createDetachedTool(MinecraftInstance* instance, QObject* parent = nullptr);
};
