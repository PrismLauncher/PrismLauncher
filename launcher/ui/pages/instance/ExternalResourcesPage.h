#pragma once

#include <QMainWindow>
#include <QSortFilterProxyModel>

#include "Application.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

class ModFolderModel;

namespace Ui {
class ExternalResourcesPage;
}

/* This page is used as a base for pages in which the user can manage external resources
 * related to the game, such as mods, shaders or resource packs. */
class ExternalResourcesPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    // FIXME: Switch to different model (or change the name of this one)
    explicit ExternalResourcesPage(BaseInstance* instance, std::shared_ptr<ModFolderModel> model, QWidget* parent = nullptr);
    virtual ~ExternalResourcesPage();

    virtual QString displayName() const override = 0;
    virtual QIcon icon() const override = 0;
    virtual QString id() const override = 0;
    virtual QString helpPage() const override = 0;

    virtual bool shouldDisplay() const override = 0;

    void openedImpl() override;
    void closedImpl() override;

    void retranslate() override;

   protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    bool listFilter(QKeyEvent* ev);
    QMenu* createPopupMenu() override;

   public slots:
    void current(const QModelIndex& current, const QModelIndex& previous);

   protected slots:
    void itemActivated(const QModelIndex& index);
    void filterTextChanged(const QString& newContents);
    virtual void runningStateChanged(bool running);

    virtual void addItem();
    virtual void removeItem();

    virtual void enableItem();
    virtual void disableItem();

    virtual void viewFolder();
    virtual void viewConfigs();

    void ShowContextMenu(const QPoint& pos);

   protected:
    BaseInstance* m_instance = nullptr;

    Ui::ExternalResourcesPage* ui = nullptr;
    std::shared_ptr<ModFolderModel> m_model;
    QSortFilterProxyModel* m_filterModel = nullptr;

    QString m_fileSelectionFilter;
    QString m_viewFilter;

    bool m_controlsEnabled = true;
};
