
#pragma once

#include <QMainWindow>
#include <memory>

#include "ui/pages/BasePage.h"

#include "Application.h"
class QStringListModel;

namespace Ui {
class InstancesDirListPage;
}

class InstancesDirListPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    inline static bool verifyInstDirPath(const QString& raw_dir);

    explicit InstancesDirListPage(QWidget* parent = 0);
    ~InstancesDirListPage();

    QString displayName() const override { return tr("External Instance"); }
    QIcon icon() const override
    {
        auto icon = APPLICATION->getThemedIcon("accounts");
        if (icon.isNull()) {
            icon = APPLICATION->getThemedIcon("noaccount");
        }
        return icon;
    }
    QString id() const override { return "external-Instance-directory"; }
    QString helpPage() const override { return "Getting-Started#adding-an-account"; }
    void retranslate() override;
    bool apply() override;
    void openedImpl() override;
   public slots:
    void on_actionRemove_triggered();
    void on_actionAddExtInst_triggered();
    void on_actionHide_triggered();

   protected slots:
    void ShowContextMenu(const QPoint& pos);

   private:
    void changeEvent(QEvent* event) override;
    QMenu* createPopupMenu() override;

    QString m_rootInstDir;
    QStringListModel* m_model;
    Ui::InstancesDirListPage* ui;
};
