#pragma once
#include "pages/BasePageProvider.h"
#include "MultiMC.h"
#include "pagedialog/PageDialog.h"
#include "InstancePageProvider.h"
#include <settings/SettingsObject.h>
#include <BaseInstance.h>

/*
 * FIXME: this is a fragment. find a better place for it.
 */
namespace SettingsUI
{
template <typename T>
void ShowPageDialog(T raw_provider, QWidget * parent, QString open_page = QString())
{
	auto provider = std::dynamic_pointer_cast<BasePageProvider>(raw_provider);
	if(!provider)
		return;
	{
		SettingsObject::Lock lock(MMC->settings());
		PageDialog dlg(provider, open_page, parent);
		dlg.exec();
	}
}

void ShowInstancePageDialog(InstancePtr instance, QWidget * parent, QString open_page = QString());
}
