#pragma once

#include "QListWidget"
#include <modplatform/PackHelpers.h>

class FtbModpackListItem : public QListWidgetItem {

private:
	FtbModpack modpack;

public:
	FtbModpackListItem(QListWidget *list, FtbModpack modpack);
	FtbModpack getModpack();

};
