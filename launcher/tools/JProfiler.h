#pragma once

#include "BaseProfiler.h"

class JProfilerFactory : public BaseProfilerFactory {
   public:
    QString name() const override { return "JProfiler"; }
    void registerSettings(SettingsObject* settings) override;
    BaseExternalTool* createTool(BaseInstance* instance, QObject* parent = 0) override;
    bool check(QString* error) override;
    bool check(const QString& path, QString* error) override;
};
