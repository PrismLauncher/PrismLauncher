#include "BisectController.h"
#include "minecraft/mod/Resource.h"

#include <algorithm>

BisectController::BisectController(BaseInstance* instance,
                                   ModFolderModel* model,
                                   QList<Mod*> lockedMods,
                                   QList<Mod*> candidateMods,
                                   QObject* parent)
    : QObject(parent), m_instance(instance), m_model(model)
{
    for (auto* m : lockedMods) {
        m_locked << m->mod_id();
    }
    for (auto* m : candidateMods) {
        m_pool << m->mod_id();
    }
}

Mod* BisectController::resolveByModId(const QString& modId) const
{
    for (int row = 0; row < m_model->rowCount({}); ++row) {
        auto& res = m_model->at(row);
        auto* mod = static_cast<Mod*>(&res);
        if (mod->mod_id() == modId) {
            return mod;
        }
    }
    return nullptr;
}

void BisectController::start()
{
    m_running = true;
    m_phase = Phase::BaselineEmpty;
    applyState({});
}

void BisectController::cancel()
{
    m_running = false;
}

void BisectController::applyState(const QSet<QString>& enabledIdsFromPool)
{
    QSet<Mod*> toEnable;
    for (const auto& id : m_locked) {
        if (auto* mod = resolveByModId(id)) {
            toEnable << mod;
        }
    }
    for (const auto& id : enabledIdsFromPool) {
        if (auto* mod = resolveByModId(id)) {
            toEnable << mod;
        }
    }

    QSet<Mod*> toDisable;
    for (const auto& id : m_pool) {
        if (!enabledIdsFromPool.contains(id)) {
            if (auto* mod = resolveByModId(id)) {
                toDisable << mod;
            }
        }
    }

    m_model->setResourceEnabledSilent(toDisable, EnableAction::DISABLE);
    m_model->setResourceEnabledSilent(toEnable, EnableAction::ENABLE);

    emit readyToLaunch();
}

void BisectController::onLaunchEnded(BaseInstance* finishedInstance, bool /*wasSuccessful*/)
{
    if (!m_running || finishedInstance != m_instance)
        return;
    emit promptUser();
}

QString BisectController::canonicalKey(const QList<QString>& ids)
{
    QStringList sorted = ids;
    sorted.sort();
    return sorted.join(',');
}

void BisectController::onUserAnswered(Answer answer)
{
    if (!m_running)
        return;

    if (answer == Answer::Relaunch) {
        emit readyToLaunch();
        return;
    }

    bool issueOccurred = (answer == Answer::Yes);

    QList<QString> currentlyEnabled;
    for (const auto& id : m_pool) {
        if (auto* mod = resolveByModId(id); mod && mod->enabled()) {
            currentlyEnabled << id;
        }
    }
    auto key = canonicalKey(currentlyEnabled);
    if (m_history.contains(key) && m_history[key] != issueOccurred) {
        m_running = false;
        emit heisenbugDetected(key);
        return;
    }
    m_history[key] = issueOccurred;

    switch (m_phase) {
        case Phase::BaselineEmpty: {
            if (issueOccurred) {
                m_running = false;
                emit bailedOut(
                    tr("Issue still occurs with all candidate mods disabled — it isn't "
                       "caused by any unlocked mod. Check locked mods or the modloader/base game."));
                return;
            }
            m_phase = Phase::BaselineFull;
            m_failing = m_pool;
            applyState(QSet<QString>(m_pool.begin(), m_pool.end()));
            return;
        }
        case Phase::BaselineFull: {
            if (!issueOccurred) {
                m_running = false;
                emit bailedOut(
                    tr("Issue did not reproduce with all candidate mods enabled — "
                       "the bug may be intermittent. Try again, or check locked mods."));
                return;
            }
            m_phase = Phase::Ddmin;
            m_granularity = 2;
            beginRound();
            return;
        }
        case Phase::Ddmin: {
            advance(issueOccurred);
            return;
        }
        case Phase::Verifying: {
            m_running = false;
            if (issueOccurred) {
                QList<Mod*> culprits;
                for (const auto& id : m_failing) {
                    if (auto* mod = resolveByModId(id))
                        culprits << mod;
                }
                emit finished(culprits);
            } else {
                emit heisenbugDetected(canonicalKey(m_failing));
            }
            return;
        }
    }
}

QList<QList<QString>> BisectController::splitIntoChunks(const QList<QString>& ids, int granularity)
{
    QList<QList<QString>> chunks;
    int n = ids.size();
    granularity = std::clamp(granularity, 2, std::max(2, n));
    int base = n / granularity, rem = n % granularity, idx = 0;
    for (int i = 0; i < granularity; ++i) {
        int size = base + (i < rem ? 1 : 0);
        QList<QString> chunk;
        for (int j = 0; j < size && idx < n; ++j, ++idx)
            chunk << ids[idx];
        if (!chunk.isEmpty())
            chunks << chunk;
    }
    return chunks;
}

void BisectController::beginRound()
{
    if (m_failing.size() <= 1) {
        m_phase = Phase::Verifying;
        applyState(QSet<QString>(m_failing.begin(), m_failing.end()));
        return;
    }
    m_chunks = splitIntoChunks(m_failing, m_granularity);
    m_subphase = Subphase::Complements;
    m_chunkIndex = 0;
    QSet<QString> complement(m_failing.begin(), m_failing.end());
    for (const auto& id : m_chunks[0])
        complement.remove(id);
    applyState(complement);
}

void BisectController::advance(bool issueOccurred)
{
    if (issueOccurred) {
        if (m_subphase == Subphase::Complements) {
            QSet<QString> newFailing(m_failing.begin(), m_failing.end());
            for (const auto& id : m_chunks[m_chunkIndex])
                newFailing.remove(id);
            m_failing = QList<QString>(newFailing.begin(), newFailing.end());
            m_granularity = std::max(2, m_granularity - 1);
        } else {
            m_failing = m_chunks[m_chunkIndex];
            m_granularity = 2;
        }
        beginRound();
        return;
    }

    ++m_chunkIndex;
    if (m_chunkIndex < m_chunks.size()) {
        if (m_subphase == Subphase::Complements) {
            QSet<QString> complement(m_failing.begin(), m_failing.end());
            for (const auto& id : m_chunks[m_chunkIndex])
                complement.remove(id);
            applyState(complement);
        } else {
            applyState(QSet<QString>(m_chunks[m_chunkIndex].begin(), m_chunks[m_chunkIndex].end()));
        }
        return;
    }

    if (m_subphase == Subphase::Complements) {
        m_subphase = Subphase::Singles;
        m_chunkIndex = 0;
        applyState(QSet<QString>(m_chunks[0].begin(), m_chunks[0].end()));
        return;
    }

    if (m_granularity < m_failing.size()) {
        m_granularity = static_cast<int>(std::min<qsizetype>(m_failing.size(), m_granularity * 2));
        beginRound();
    } else {
        m_phase = Phase::Verifying;
        applyState(QSet<QString>(m_failing.begin(), m_failing.end()));
    }
}
