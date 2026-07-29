#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <memory>

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

class BisectController : public QObject {
    Q_OBJECT
   public:
    enum class Answer { Yes, No, Relaunch };

    BisectController(BaseInstance* instance,
                     ModFolderModel* model,
                     QList<Mod*> lockedMods,
                     QList<Mod*> candidateMods,
                     QObject* parent = nullptr);

    void start();
    void cancel();
    [[nodiscard]] bool isRunning() const { return m_running; }

   public slots:
    void onLaunchEnded(BaseInstance* finishedInstance, bool wasSuccessful);
    void onUserAnswered(Answer answer);

   signals:
    void readyToLaunch();
    void promptUser();
    void bailedOut(QString reason);
    void heisenbugDetected(QString conflictingSetDescription);
    void finished(QList<Mod*> culprits);

   private:
    enum class Phase { BaselineEmpty, BaselineFull, Ddmin, Verifying };
    enum class Subphase { Complements, Singles };

    void applyState(const QSet<QString>& enabledIdsFromPool);
    Mod* resolveByModId(const QString& modId) const;
    void beginRound();
    void advance(bool issueOccurred);
    static QString canonicalKey(const QList<QString>& ids);
    static QList<QList<QString>> splitIntoChunks(const QList<QString>& ids, int granularity);

    BaseInstance* m_instance;
    ModFolderModel* m_model;
    QList<QString> m_locked;
    QList<QString> m_pool;

    Phase m_phase = Phase::BaselineEmpty;
    Subphase m_subphase = Subphase::Complements;
    QList<QString> m_failing;
    int m_granularity = 2;
    int m_chunkIndex = 0;
    QList<QList<QString>> m_chunks;

    QHash<QString, bool> m_history;
    bool m_running = false;
};
