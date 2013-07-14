#ifndef INSTANCESETTINGS_H
#define INSTANCESETTINGS_H

#include <QDialog>
#include "settingsobject.h"

namespace Ui {
class InstanceSettings;
}

class InstanceSettings : public QDialog
{
    Q_OBJECT
    
public:
    explicit InstanceSettings(QWidget *parent = 0);
    ~InstanceSettings();

    void updateCheckboxStuff();

    void applySettings(SettingsObject *s);
    void loadSettings(SettingsObject* s);
    
private slots:
    void on_customCommandsGroupBox_toggled(bool arg1);

private:
    Ui::InstanceSettings *ui;
};

#endif // INSTANCESETTINGS_H
