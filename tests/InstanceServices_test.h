#pragma once

#include <QObject>

class InstanceServicesTest : public QObject {
    Q_OBJECT

   private slots:
    void nativeLibraryPathComesFromInjectedRuntime();
    void assetUrlComesFromInjectedSettings();
    void missingServicesSkipNativeLibraries();
};
