#pragma once

#include "BaseProfiler.h"

class JProfilerFactory : public BaseProfilerFactory {
   public:
    QString name() const override { return "JProfiler"; }
    void registerSettings(SettingsObject* settings) override;
    BaseExternalTool* createTool(MinecraftInstance* instance, QObject* parent = nullptr) override;
    bool check(QString* error) override;
    bool check(const QString& path, QString* error) override;
};
