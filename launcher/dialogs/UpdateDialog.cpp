#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include <QDebug>
#include "Launcher.h"
#include <settings/SettingsObject.h>
#include <Json.h>

#include "BuildConfig.h"
#include "HoeDown.h"

UpdateDialog::UpdateDialog(bool hasUpdate, QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog)
{
    ui->setupUi(this);
    auto channel = LAUNCHER->settings()->get("UpdateChannel").toString();
    if(hasUpdate)
    {
        ui->label->setText(tr("A new %1 update is available!").arg(channel));
    }
    else
    {
        ui->label->setText(tr("No %1 updates found. You are running the latest version.").arg(channel));
        ui->btnUpdateNow->setHidden(true);
        ui->btnUpdateLater->setText(tr("Close"));
    }
    ui->changelogBrowser->setHtml(tr("<center><h1>Loading changelog...</h1></center>"));
    loadChangelog();
    restoreGeometry(QByteArray::fromBase64(LAUNCHER->settings()->get("UpdateDialogGeometry").toByteArray()));
}

UpdateDialog::~UpdateDialog()
{
}

void UpdateDialog::loadChangelog()
{
    auto channel = LAUNCHER->settings()->get("UpdateChannel").toString();
    dljob.reset(new NetJob("Changelog"));
    QString url;
    if(channel == "stable")
    {
        url = QString("https://raw.githubusercontent.com/MultiMC/MultiMC5/%1/changelog.md").arg(channel);
        m_changelogType = CHANGELOG_MARKDOWN;
    }
    else
    {
        url = QString("https://api.github.com/repos/MultiMC/MultiMC5/compare/%1...%2").arg(BuildConfig.GIT_COMMIT, channel);
        m_changelogType = CHANGELOG_COMMITS;
    }
    dljob->addNetAction(Net::Download::makeByteArray(QUrl(url), &changelogData));
    connect(dljob.get(), &NetJob::succeeded, this, &UpdateDialog::changelogLoaded);
    connect(dljob.get(), &NetJob::failed, this, &UpdateDialog::changelogFailed);
    dljob->start();
}

QString reprocessMarkdown(QByteArray markdown)
{
    HoeDown hoedown;
    QString output = hoedown.process(markdown);

    // HACK: easier than customizing hoedown
    output.replace(QRegExp("GH-([0-9]+)"), "<a href=\"https://github.com/MultiMC/MultiMC5/issues/\\1\">GH-\\1</a>");
    qDebug() << output;
    return output;
}

QString reprocessCommits(QByteArray json)
{
    auto channel = LAUNCHER->settings()->get("UpdateChannel").toString();
    try
    {
        QString result;
        auto document = Json::requireDocument(json);
        auto rootobject = Json::requireObject(document);
        auto status = Json::requireString(rootobject, "status");
        auto diff_url = Json::requireString(rootobject, "html_url");

        auto print_commits = [&]()
        {
            result += "<table cellspacing=0 cellpadding=2 style='border-width: 1px; border-style: solid'>";
            auto commitarray = Json::requireArray(rootobject, "commits");
            for(int i = commitarray.size() - 1; i >= 0; i--)
            {
                const auto & commitval = commitarray[i];
                auto commitobj = Json::requireObject(commitval);
                auto parents_info = Json::ensureArray(commitobj, "parents");
                // NOTE: this ignores merge commits, because they have more than one parent
                if(parents_info.size() > 1)
                {
                    continue;
                }
                auto commit_url = Json::requireString(commitobj, "html_url");
                auto commit_info = Json::requireObject(commitobj, "commit");
                auto commit_message = Json::requireString(commit_info, "message");
                auto lines = commit_message.split('\n');
                QRegularExpression regexp("(?<prefix>(GH-(?<issuenr>[0-9]+))|(NOISSUE)|(SCRATCH))? *(?<rest>.*) *");
                auto match = regexp.match(lines.takeFirst(), 0, QRegularExpression::NormalMatch);
                auto issuenr = match.captured("issuenr");
                auto prefix = match.captured("prefix");
                auto rest = match.captured("rest");
                result += "<tr><td>";
                if(issuenr.length())
                {
                    result += QString("<a href=\"https://github.com/MultiMC/MultiMC5/issues/%1\">GH-%2</a>").arg(issuenr, issuenr);
                }
                else if(prefix.length())
                {
                    result += QString("<a href=\"%1\">%2</a>").arg(commit_url, prefix);
                }
                else
                {
                    result += QString("<a href=\"%1\">NOISSUE</a>").arg(commit_url);
                }
                result += "</td>";
                lines.prepend(rest);
                result += "<td><p>" + lines.join("<br />") + "</p></td></tr>";
            }
            result += "</table>";
        };

        if(status == "identical")
        {
            return QObject::tr("<p>There are no code changes between your current version and latest %1.</p>").arg(channel);
        }
        else if(status == "ahead")
        {
            result += QObject::tr("<p>Following commits were added since last update:</p>");
            print_commits();
        }
        else if(status == "diverged")
        {
            auto commit_ahead = Json::requireInteger(rootobject, "ahead_by");
            auto commit_behind = Json::requireInteger(rootobject, "behind_by");
            result += QObject::tr("<p>The update removes %1 commits and adds the following %2:</p>").arg(commit_behind).arg(commit_ahead);
            print_commits();
        }
        result += QObject::tr("<p>You can <a href=\"%1\">look at the changes on github</a>.</p>").arg(diff_url);
        return result;
    }
    catch (const JSONValidationError &e)
    {
        qWarning() << "Got an unparseable commit log from github:" << e.what();
        qDebug() << json;
    }
    return QString();
}

void UpdateDialog::changelogLoaded()
{
    QString result;
    switch(m_changelogType)
    {
        case CHANGELOG_COMMITS:
            result = reprocessCommits(changelogData);
            break;
        case CHANGELOG_MARKDOWN:
            result = reprocessMarkdown(changelogData);
            break;
    }
    changelogData.clear();
    ui->changelogBrowser->setHtml(result);
}

void UpdateDialog::changelogFailed(QString reason)
{
    ui->changelogBrowser->setHtml(tr("<p align=\"center\" <span style=\"font-size:22pt;\">Failed to fetch changelog... Error: %1</span></p>").arg(reason));
}

void UpdateDialog::on_btnUpdateLater_clicked()
{
    reject();
}

void UpdateDialog::on_btnUpdateNow_clicked()
{
    done(UPDATE_NOW);
}

void UpdateDialog::closeEvent(QCloseEvent* evt)
{
    LAUNCHER->settings()->set("UpdateDialogGeometry", saveGeometry().toBase64());
    QDialog::closeEvent(evt);
}
