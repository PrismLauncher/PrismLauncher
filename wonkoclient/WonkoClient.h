//
// Created by robotbrain on 3/27/16.
//

#pragma once

#include <memory>
#include <InstanceList.h>

#if defined(MMCC)
#undef MMCC
#endif
#define MMCC (WonkoClient::getInstance())

class WonkoClient : public QObject {
Q_OBJECT

private:
    WonkoClient();

public:
    static WonkoClient &getInstance();

    void registerLists();
    void initGlobalSettings();

    std::shared_ptr<InstanceList> instances() const {
        return m_instanceList;
    }

private:
    std::shared_ptr<InstanceList> m_instanceList;
    std::shared_ptr<SettingsObject> m_settings;

    void runTask(Task *pTask);
};
