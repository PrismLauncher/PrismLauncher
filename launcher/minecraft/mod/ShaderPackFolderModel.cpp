#include "ShaderPackFolderModel.h"
#include "FileSystem.h"

namespace {
class ShaderPackIndexMigrateTask : public Task {
    Q_OBJECT
   public:
    ShaderPackIndexMigrateTask(QDir resourceDir, QDir indexDir) : m_resourceDir(std::move(resourceDir)), m_indexDir(std::move(indexDir)) {}

    void executeTask() override
    {
        if (!m_indexDir.exists()) {
            qDebug() << m_indexDir.absolutePath() << "does not exist; nothing to migrate";
            emitSucceeded();
            return;
        }

        QStringList pwFiles = m_indexDir.entryList({ "*.pw.toml" }, QDir::Files);
        bool movedAll = true;

        for (const auto& file : pwFiles) {
            QString src = m_indexDir.filePath(file);
            QString dest = m_resourceDir.filePath(file);

            if (FS::move(src, dest)) {
                qDebug() << "Moved" << src << "to" << dest;
            } else {
                movedAll = false;
            }
        }

        if (!movedAll) {
            // FIXME: not shown in the UI
            emitFailed(tr("Failed to migrate shaderpack metadata from .index"));
            return;
        }

        if (!FS::deletePath(m_indexDir.absolutePath())) {
            emitFailed(tr("Failed to remove old .index dir"));
            return;
        }

        emitSucceeded();
    }

   private:
    QDir m_resourceDir, m_indexDir;
};

QString versionSuffix(const QString& name, const QString& versionNumber)
{
    auto nameWords = name.split(' ', Qt::SkipEmptyParts);
    QStringList versionWords;

    for (const auto& word : versionNumber.split(' ', Qt::SkipEmptyParts)) {
        if (!nameWords.contains(word, Qt::CaseInsensitive))
            versionWords << word;
    }

    return versionWords.join(' ');
}
}  // namespace

QVariant ShaderPackFolderModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole && index.column() == NameColumn && validateIndex(index)) {
        if (auto metadata = m_resources[index.row()]->metadata(); metadata && !metadata->version_number.isEmpty()) {
            auto suffix = versionSuffix(metadata->name, metadata->version_number);
            if (!suffix.isEmpty())
                return QString("%1 %2").arg(metadata->name, suffix);
        }
    }

    return ResourceFolderModel::data(index, role);
}

Task* ShaderPackFolderModel::createPreUpdateTask()
{
    return new ShaderPackIndexMigrateTask(m_dir, ResourceFolderModel::indexDir());
}

#include "ShaderPackFolderModel.moc"
