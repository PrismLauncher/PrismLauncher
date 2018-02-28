#include "FtbModpackListItem.h"

FtbModpackListItem::FtbModpackListItem(QListWidget *list, FtbModpack modpack) : QListWidgetItem(list), modpack(modpack) {
}

FtbModpack FtbModpackListItem::getModpack(){
	return modpack;
}
