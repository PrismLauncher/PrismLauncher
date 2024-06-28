#pragma once

class BasePage;

class BasePageContainer {
   public:
    virtual ~BasePageContainer() {};
    virtual bool selectPage(QString pageId) = 0;
    virtual BasePage* selectedPage() const = 0;
    virtual BasePage* getPage(QString pageId) { return nullptr; };
    virtual void refreshContainer() = 0;
    virtual bool requestClose() = 0;
};
