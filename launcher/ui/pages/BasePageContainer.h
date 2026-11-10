#pragma once

#include <QString>

class BasePage;
class QWidget;

class BasePageContainer {
   public:
    virtual ~BasePageContainer() {};
    virtual bool selectPage(QString pageId) = 0;
    virtual QWidget* selectedPageWidget() const = 0;
    virtual QWidget* pageWidget(QString /*pageId*/) const { return nullptr; };
    virtual void refreshContainer() = 0;
    virtual bool requestClose() = 0;
};
