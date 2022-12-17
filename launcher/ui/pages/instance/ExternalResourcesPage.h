#pragma once

#include <QMainWindow>
#include <QSortFilterProxyModel>

#include "Application.h"
#include "settings/Setting.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"

class ResourceFolderModel;

namespace Ui {
class ExternalResourcesPage;
}

/* This page is used as a base for pages in which the user can manage external resources
 * related to the game, such as mods, shaders or resource packs. */
class ExternalResourcesPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    explicit ExternalResourcesPage(BaseInstance* instance, std::shared_ptr<ResourceFolderModel> model, QWidget* parent = nullptr);
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
    bool current(const QModelIndex& current, const QModelIndex& previous);

    virtual bool onSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

   protected slots:
    void itemActivated(const QModelIndex& index);
    void filterTextChanged(const QString& newContents);

    virtual void addItem();
    virtual void removeItem();

    virtual void enableItem();
    virtual void disableItem();

    virtual void viewFolder();
    virtual void viewConfigs();

    void ShowContextMenu(const QPoint& pos);

   protected:
    BaseInstance* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance = nullptr;

    Ui::ExternalResourcesPage* ui = nullptr;
    std::shared_ptr<ResourceFolderModel> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model;
    QSortFilterProxyModel* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel = nullptr;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fileSelectionFilter;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_viewFilter;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled = true;

    std::shared_ptr<Setting> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_wide_bar_setting = nullptr;
};
