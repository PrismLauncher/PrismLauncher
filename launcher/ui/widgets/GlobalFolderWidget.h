#pragma once

#include <QWidget>

namespace Ui {
class GlobalFolderWidget;
}

class GlobalFolderWidget : public QWidget {
    Q_OBJECT

   public:
    explicit GlobalFolderWidget(QWidget* parent = 0);
    virtual ~GlobalFolderWidget();
    void initialize(bool enabled, const QString& path, const QString& label = "");

    bool isEnabled() const;
    QString getPath() const;

   private slots:
    void on_enabledCheckBox_toggled(bool checked);
    void on_pathBrowseBtn_clicked();

   private:
    Ui::GlobalFolderWidget* ui;
};
