#pragma once

#include <QWidget>

namespace Ui {
class SharedFolderWidget;
}

class SharedFolderWidget : public QWidget {
    Q_OBJECT

   public:
    explicit SharedFolderWidget(QWidget* parent = 0);
    virtual ~SharedFolderWidget();
    void initialize(bool enabled, const QString& path, const QString& label = "");

    bool isEnabled() const;
    QString getPath() const;

   private slots:
    void on_enabledCheckBox_toggled(bool checked);
    void on_pathBrowseBtn_clicked();

   private:
    Ui::SharedFolderWidget* ui;
};
