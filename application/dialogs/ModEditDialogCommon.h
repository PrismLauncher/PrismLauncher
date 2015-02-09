#pragma once
#include <QModelIndex>
#include <QDesktopServices>
#include <QWidget>
#include <minecraft/Mod.h>

bool lastfirst(QModelIndexList &list, int &first, int &last);

void showWebsiteForMod(QWidget *parentDlg, Mod &m);
