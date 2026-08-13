#pragma once

#include <BaseInstance.h>
#include <QObject>

class BaseInstance;
class SettingsObject;
class QProcess;

class BaseExternalTool : public QObject {
    Q_OBJECT
   public:
    explicit BaseExternalTool(SettingsObject* settings, BaseInstance* instance, QObject* parent = 0);
    virtual ~BaseExternalTool();

   protected:
    BaseInstance* m_instance;
    SettingsObject* globalSettings;
};

class BaseExternalToolFactory {
   public:
    virtual ~BaseExternalToolFactory();

    virtual QString name() const = 0;

    virtual void registerSettings(SettingsObject* settings) = 0;

    virtual BaseExternalTool* createTool(BaseInstance* instance, QObject* parent = 0) = 0;

    virtual bool check(QString* error) = 0;
    virtual bool check(const QString& path, QString* error) = 0;

   protected:
    SettingsObject* globalSettings;
};
