#include "DefaultGameOptionsPage.h"
#include "ui_DefaultGameOptionsPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/gameoptions/GameOptions.h"
#include <QTabBar>
#include "MultiMC.h"
namespace {
enum Mode {
    NoDefault = 0,
    NoAutojump = 1,
    Fulltext = 2
};
}

DefaultGameOptionsPage::DefaultGameOptionsPage(QWidget* parent) : QWidget(parent), ui(new Ui::DefaultGameOptionsPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    ui->defaultOptionsMode->setId(ui->radioDisabled, NoDefault);
    ui->defaultOptionsMode->setId(ui->radioNoAutojump, NoAutojump);
    ui->defaultOptionsMode->setId(ui->radioFullText, Fulltext);
    loadSettings();
    updateEnabledWidgets();
    connect(ui->defaultOptionsMode, SIGNAL(buttonClicked(int)), SLOT(radioChanged(int)));
}

bool DefaultGameOptionsPage::apply()
{
    applySettings();
    return true;
}

void DefaultGameOptionsPage::updateEnabledWidgets()
{
    auto id = ui->defaultOptionsMode->checkedId();
    switch(id) {
        case NoDefault:
        default:
        case NoAutojump: {
            ui->textEdit->setEnabled(false);
            break;
        }
        case Fulltext: {
            ui->textEdit->setEnabled(true);
            break;
        }
    }
}

void DefaultGameOptionsPage::radioChanged(int)
{
    updateEnabledWidgets();
}


void DefaultGameOptionsPage::applySettings()
{
    auto s = MMC->settings();

    auto id = ui->defaultOptionsMode->checkedId();
    switch(id) {
        case NoDefault: {
            s->set("DefaultOptionsMode", "NoDefault");
            break;
        }
        default:
        case NoAutojump: {
            s->set("DefaultOptionsMode", "NoAutojump");
            break;
        }
        case Fulltext: {
            s->set("DefaultOptionsMode", "Fulltext");
            break;
        }
    }

    s->set("DefaultOptionsText", ui->textEdit->toPlainText());
}

void DefaultGameOptionsPage::loadSettings()
{
    auto s = MMC->settings();
    auto modeStr = s->get("DefaultOptionsMode").toString();
    if(modeStr == "NoDefault") {
        ui->radioDisabled->setChecked(true);
    } else if(modeStr == "Fulltext") {
        ui->radioFullText->setChecked(true);
    } else {
        ui->radioNoAutojump->setChecked(true);
    }
    ui->textEdit->setText(s->get("DefaultOptionsText").toString());
}


DefaultGameOptionsPage::~DefaultGameOptionsPage()
{
    delete ui;
}

void DefaultGameOptionsPage::openedImpl()
{
}

void DefaultGameOptionsPage::closedImpl()
{
}

#include "DefaultGameOptionsPage.moc"


