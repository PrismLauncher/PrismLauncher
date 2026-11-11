#include <QComboBox>
#include <QFileDialog>

#include "FileSystemAccessWidget.h"
#include "ui_FileSystemAccessWidget.h"

FileSystemAccessWidget::FileSystemAccessWidget(QWidget* parent) : m_ui(new Ui::FileSystemAccessWidget)
{
    m_ui->setupUi(this);

    auto* header = m_ui->list->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    // header->resizeSection(1, 80);

    connect(m_ui->addButton, &QPushButton::clicked, this, [this] {
        const auto dir = QFileDialog::getExistingDirectory(this, {}, m_defaultDir);
        if (!dir.isEmpty()) {
            addItem(dir);
            m_ui->list->selectionModel()->select(m_ui->list->model()->index(m_ui->list->topLevelItemCount() - 1, 0),
                                                 QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
    });
    connect(m_ui->removeButton, &QPushButton::clicked, this, [this] {
        for (QTreeWidgetItem* item : m_ui->list->selectedItems()) {
            m_ui->list->takeTopLevelItem(m_ui->list->indexOfTopLevelItem(item));
        }
    });
    connect(m_ui->clearButton, &QPushButton::clicked, m_ui->list, &QTreeWidget::clear);
}

FileSystemAccessWidget::~FileSystemAccessWidget()
{
    delete m_ui;
}

void FileSystemAccessWidget::load(const QVariantMap& map)
{
    m_ui->list->clear();
    for (const auto& [key, value] : map.asKeyValueRange()) {
        addItem(key, value.toString());
    }
}

QVariantMap FileSystemAccessWidget::save() const
{
    QVariantMap result;
    for (int i = 0; i < m_ui->list->topLevelItemCount(); ++i) {
        auto* item = m_ui->list->topLevelItem(i);

        auto* comboBox = dynamic_cast<QComboBox*>(m_ui->list->itemWidget(item, 1));
        Q_ASSERT(comboBox != nullptr);

        const auto& path = item->text(0);
        if (path.isEmpty()) {
            continue;
        }

        result.insert(item->text(0), comboBox->currentData());
    }

    return result;
}

QTreeWidgetItem* FileSystemAccessWidget::addItem(const QString& path, const QString& access)
{
    auto* item = new QTreeWidgetItem(m_ui->list);
    item->setText(0, path);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_ui->list->addTopLevelItem(item);

    auto* comboBox = new QComboBox(m_ui->list);

    comboBox->addItem(tr("Read-only"), "ro");
    comboBox->addItem(tr("Read/Write"), "rw");

    if (access == "rw") {
        comboBox->setCurrentIndex(1);
    } else {
        comboBox->setCurrentIndex(0);
    }

    m_ui->list->setItemWidget(item, 1, comboBox);
    return item;
}