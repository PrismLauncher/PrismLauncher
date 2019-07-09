#include "UrlResolvingTask.h"
#include <QtXml>
#include <Json.h>


namespace {
    const char * metabase = "https://cursemeta.dries007.net";
}

Flame::UrlResolvingTask::UrlResolvingTask(const QString& toProcess)
    : m_url(toProcess)
{
}

void Flame::UrlResolvingTask::executeTask()
{
    resolveUrl();
}

void Flame::UrlResolvingTask::resolveUrl()
{
    setStatus(tr("Resolving URL..."));
    setProgress(0, 1);
    QUrl actualUrl(m_url);
    if(actualUrl.host() != "www.curseforge.com") {
        emitFailed(tr("Not a Twitch URL."));
        return;
    }
    m_dljob.reset(new NetJob("URL resolver"));

    bool weAreDigging = false;
    needle = QString();

    if(m_url.startsWith("https://")) {
        if(m_url.endsWith("?client=y")) {
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download?client=y
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download/2697088?client=y
            m_url.chop(9);
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download/2697088
        }
        if(m_url.endsWith("/download")) {
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download -> need to dig inside html...
            weAreDigging = true;
            needle = m_url;
            needle.replace("https://", "twitch://");
            needle.replace("/download", "/download-client/");
            m_url.append("?client=y");
        } else if (m_url.contains("/download/")) {
            // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download/2697088
            m_url.replace("/download/", "/download-client/");
        }
    }
    else if(m_url.startsWith("twitch://")) {
        // twitch://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download-client/2697088
        m_url.replace(0, 9, "https://");
        // https://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download-client/2697088
    }
    auto dl = Net::Download::makeByteArray(QUrl(m_url), &results);
    m_dljob->addNetAction(dl);
    if(weAreDigging) {
        connect(m_dljob.get(), &NetJob::finished, this, &Flame::UrlResolvingTask::processHTML);
    } else {
        connect(m_dljob.get(), &NetJob::finished, this, &Flame::UrlResolvingTask::processCCIP);
    }
    m_dljob->start();
}

void Flame::UrlResolvingTask::processHTML()
{
    QString htmlDoc = QString::fromUtf8(results);
    auto index = htmlDoc.indexOf(needle);
    if(index < 0) {
        emitFailed(tr("Couldn't find the needle in the haystack..."));
        return;
    }
    auto indexStart = index;
    int indexEnd = -1;
    while((index + 1) < htmlDoc.size() && htmlDoc[index] != '"') {
        index ++;
        if(htmlDoc[index] == '"') {
            indexEnd = index;
            break;
        }
    }
    if(indexEnd > 0) {
        QString found = htmlDoc.mid(indexStart, indexEnd - indexStart);
        qDebug() << "Found needle: " << found;
        // twitch://www.curseforge.com/minecraft/modpacks/ftb-sky-odyssey/download-client/2697088
        m_url = found;
        resolveUrl();
        return;
    }
    emitFailed(tr("Couldn't find the end of the needle in the haystack..."));
    return;
}

void Flame::UrlResolvingTask::processCCIP()
{
    QDomDocument doc;
    if (!doc.setContent(results)) {
        qDebug() << results;
        emitFailed(tr("Resolving failed."));
        return;
    }
    auto packageNode = doc.namedItem("package");
    if(!packageNode.isElement()) {
        emitFailed(tr("Resolving failed: missing package root element."));
        return;
    }
    auto projectNode = packageNode.namedItem("project");
    if(!projectNode.isElement()) {
        emitFailed(tr("Resolving failed: missing project element."));
        return;
    }
    auto attribs = projectNode.attributes();

    auto projectIdNode = attribs.namedItem("id");
    if(!projectIdNode.isAttr()) {
        emitFailed(tr("Resolving failed: missing id attribute."));
        return;
    }
    auto fileIdNode = attribs.namedItem("file");
    if(!fileIdNode.isAttr()) {
        emitFailed(tr("Resolving failed: missing file attribute."));
        return;
    }

    auto projectId = projectIdNode.nodeValue();
    auto fileId = fileIdNode.nodeValue();
    bool success = true;
    m_result.projectId = projectId.toInt(&success);
    if(!success) {
        emitFailed(tr("Failed to resove projectId as a number."));
        return;
    }
    m_result.fileId = fileId.toInt(&success);
    if(!success) {
        emitFailed(tr("Failed to resove fileId as a number."));
        return;
    }
    qDebug() << "Resolved" << m_url << "as" << m_result.projectId << "/" << m_result.fileId;
    resolveIDs();
}

void Flame::UrlResolvingTask::resolveIDs()
{
    setStatus(tr("Resolving mod IDs..."));
    m_dljob.reset(new NetJob("Mod id resolver"));
    auto projectIdStr = QString::number(m_result.projectId);
    auto fileIdStr = QString::number(m_result.fileId);
    QString metaurl = QString("%1/%2/%3.json").arg(metabase, projectIdStr, fileIdStr);
    auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results);
    m_dljob->addNetAction(dl);
    connect(m_dljob.get(), &NetJob::finished, this, &Flame::UrlResolvingTask::processCursemeta);
    m_dljob->start();
}

void Flame::UrlResolvingTask::processCursemeta()
{
    try {
        if(m_result.parseFromBytes(results)) {
            emitSucceeded();
            qDebug() << results;
            return;
        }
    } catch (const JSONValidationError &e) {

        qCritical() << "Resolving of" << m_result.projectId << m_result.fileId << "failed because of a parsing error:";
        qCritical() << e.cause();
        qCritical() << "JSON:";
        qCritical() << results;
    }
    emitFailed(tr("Failed to resolve the modpack file."));
}
