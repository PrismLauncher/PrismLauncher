#pragma once

class BasePageContainer
{
public:
	virtual ~BasePageContainer(){};
	virtual bool selectPage(QString pageId) = 0;
};
