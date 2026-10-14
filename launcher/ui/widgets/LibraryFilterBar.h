// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QList>
#include <QString>
#include <QWidget>
#include <optional>
#include <utility>

class QButtonGroup;
class QComboBox;
class QHBoxLayout;
class QLineEdit;
class QToolButton;

// Search field, group chips and sort order shown above the instance grid
class LibraryFilterBar : public QWidget {
    Q_OBJECT

   public:
    // A group name and how many instances it holds. An empty name is the "Ungrouped" group.
    using GroupCount = std::pair<QString, int>;

    explicit LibraryFilterBar(QWidget* parent = nullptr);

    // Rebuilds the group chips; keeps the current choice if that group still exists
    void setGroups(const QList<GroupCount>& groups, int total);
    void setSortMode(const QString& mode);
    void focusSearch();

    QString searchText() const;
    // No value: every group
    std::optional<QString> group() const { return m_group; }

   signals:
    void searchChanged(const QString& text);
    void groupChanged(std::optional<QString> group);
    void sortModeChanged(const QString& mode);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    void rebuildChips();
    QToolButton* addChip(const QString& label, int count, std::optional<QString> group);
    void selectGroup(std::optional<QString> group);
    void updateChipStates();
    void retranslate();

    QLineEdit* m_search;
    QHBoxLayout* m_chipLayout;
    QButtonGroup* m_chips;
    QComboBox* m_sort;
    QList<GroupCount> m_groups;
    int m_total = 0;
    std::optional<QString> m_group;
};
