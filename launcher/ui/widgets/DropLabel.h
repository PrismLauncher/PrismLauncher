#pragma once

#include <QLabel>

class DropLabel : public QLabel {
    Q_OBJECT

   public:
    explicit DropLabel(QWidget* parent = nullptr);

   signals:
    void droppedURLs(QList<QUrl> urls);

   protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
};
