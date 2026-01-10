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
}  // namespace

Task* ShaderPackFolderModel::createPreUpdateTask()
{
    return new ShaderPackIndexMigrateTask(m_dir, ResourceFolderModel::indexDir());
}

#include "ShaderPackFolderModel.moc"
