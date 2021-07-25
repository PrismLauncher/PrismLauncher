#include "CustomCommandsPage.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>

CustomCommandsPage::CustomCommandsPage(QWidget* parent): QWidget(parent)
{

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    auto tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("tabWidget"));
    commands = new CustomCommands(this);
    commands->setContentsMargins(6, 6, 6, 6);
    tabWidget->addTab(commands, "Foo");
    tabWidget->tabBar()->hide();
    verticalLayout->addWidget(tabWidget);
    loadSettings();
}

CustomCommandsPage::~CustomCommandsPage()
{
}

bool CustomCommandsPage::apply()
{
    applySettings();
    return true;
}

void CustomCommandsPage::applySettings()
{
    auto s = MMC->settings();
    s->set("PreLaunchCommand", commands->prelaunchCommand());
    s->set("WrapperCommand", commands->wrapperCommand());
    s->set("PostExitCommand", commands->postexitCommand());
}

void CustomCommandsPage::loadSettings()
{
    auto s = MMC->settings();
    commands->initialize(
        false,
        true,
        s->get("PreLaunchCommand").toString(),
        s->get("WrapperCommand").toString(),
        s->get("PostExitCommand").toString()
    );
}
