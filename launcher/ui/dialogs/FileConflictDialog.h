#pragma once

#include <QDialog>

namespace Ui {
class FileConflictDialog;
}

class FileConflictDialog : public QDialog {
    Q_OBJECT

   public:
    enum Result { Cancel, ChooseSource, ChooseDestination };

    /// @brief Create a new file conflict dialog
    /// @param source      The source path. What to copy/move.
    /// @param destination The destination path. Where to copy/move.
    /// @param move        Whether the conflict is for a move or copy action
    /// @param parent      The parent of the dialog
    explicit FileConflictDialog(QString source, QString destination, bool move = false, QWidget* parent = nullptr);
    ~FileConflictDialog() override;

    Result execWithResult();
    Result getResult() const;

   private slots:
    void chooseSource();
    void chooseDestination();
    void cancel();

   private:
    QString GetFileInfoText(const QString& filePath) const;

    Ui::FileConflictDialog* ui;
    Result m_result;
};
