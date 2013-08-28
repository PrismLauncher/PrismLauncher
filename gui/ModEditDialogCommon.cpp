#include "ModEditDialogCommon.h"

bool lastfirst (QModelIndexList & list, int & first, int & last)
{
	if(!list.size())
		return false;
	first = last = list[0].row();
	for(auto item: list)
	{
		int row = item.row();
		if(row < first)
			first = row;
		if(row > last)
			last = row;
	}
	return true;
}