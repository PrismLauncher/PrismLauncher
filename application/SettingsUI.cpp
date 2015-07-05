#include "SettingsUI.h"
namespace SettingsUI
{
void ShowInstancePageDialog(InstancePtr instance, QWidget * parent, QString open_page)
{
	auto provider = std::make_shared<InstancePageProvider>(instance);
	ShowPageDialog(provider, parent, open_page);
}
}
