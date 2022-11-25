#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLayout>

#include "ui/pages/BasePageProvider.h"

class ResourceDownloadTask;
class ResourcePage;
class ResourceFolderModel;
class PageContainer;
class QVBoxLayout;
class QDialogButtonBox;

class ResourceDownloadDialog : public QDialog, public BasePageProvider {
    Q_OBJECT

   public:
    ResourceDownloadDialog(QWidget* parent, const std::shared_ptr<ResourceFolderModel> base_model);

    void initializeContainer();
    void connectButtons();

    //: String that gets appended to the download dialog title ("Download " + resourcesString())
    [[nodiscard]] virtual QString resourceString() const { return tr("resources"); }

    QString dialogTitle() override { return tr("Download %1").arg(resourceString()); };

    bool selectPage(QString pageId);
    ResourcePage* getSelectedPage();

    void addResource(QString name, ResourceDownloadTask* task);
    void removeResource(QString name);
    [[nodiscard]] bool isSelected(QString name, QString filename = "") const;

    const QList<ResourceDownloadTask*> getTasks();
    [[nodiscard]] const std::shared_ptr<ResourceFolderModel> getBaseModel() const { return m_base_model; }

   protected slots:
    void selectedPageChanged(BasePage* previous, BasePage* selected);

    virtual void confirm();

   protected:
    const std::shared_ptr<ResourceFolderModel> m_base_model;

    PageContainer* m_container = nullptr;
    ResourcePage* m_selectedPage = nullptr;

    QDialogButtonBox m_buttons;
    QVBoxLayout m_vertical_layout;

    QHash<QString, ResourceDownloadTask*> m_selected;
};
