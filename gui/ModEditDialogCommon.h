#pragma once
#include <QAbstractItemModel>
#include <logic/Mod.h>

bool lastfirst (QModelIndexList & list, int & first, int & last);

void showWebsiteForMod(QWidget * parentDlg, Mod& m);