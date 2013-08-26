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
	explicit InstanceSettings(SettingsObject *s, QWidget *parent = 0);
	~InstanceSettings();

	void updateCheckboxStuff();

	void applySettings();
	void loadSettings();
protected:
	virtual void showEvent ( QShowEvent* );
private slots:
	void on_customCommandsGroupBox_toggled(bool arg1);
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

private:
	Ui::InstanceSettings *ui;
	SettingsObject * m_obj;
};

#endif // INSTANCESETTINGS_H
