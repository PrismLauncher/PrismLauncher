#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QWidget>

class QTreeWidgetItem;

namespace Ui {
class FileSystemAccessWidget;
};

class FileSystemAccessWidget : public QWidget {
    Q_OBJECT
   public:
    FileSystemAccessWidget(QWidget* parent = nullptr);

    ~FileSystemAccessWidget();

    void setDefaultDir(QString dir);

    void load(const QVariantMap& map);

    QVariantMap save() const;

   private:
    QTreeWidgetItem* addItem(const QString& path = {}, const QString& access = {});

    Ui::FileSystemAccessWidget* m_ui;
    QString m_defaultDir;
};