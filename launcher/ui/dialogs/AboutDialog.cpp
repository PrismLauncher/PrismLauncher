/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AboutDialog.h"
#include "BuildConfig.h"
#include "ui_AboutDialog.h"
#include <QIcon>
#include "Application.h"
#include "BuildConfig.h"

#include <net/NetJob.h>

#include "HoeDown.h"

namespace {
// Credits
// This is a hack, but I can't think of a better way to do this easily without screwing with QTextDocument...
QString getCreditsHtml()
{
    QString output;
    QTextStream stream(&output);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));
    stream << "<center>\n";

    //: %1 is the name of the launcher, determined at build time, e.g. "PolyMC Developers"
    stream << "<h3>" << QObject::tr("%1 Developers", "About Credits").arg(BuildConfig.LAUNCHER_NAME) << "</h3>\n";
    stream << "<p>swirl &lt;<a href='mailto:swurl@swurl.xyz'>swurl@swurl.xyz </a>&gt;</p>\n";
    stream << "<p>LennyMcLennington &lt;<a href='mailto:lenny@sneed.church'>lenny@sneed.church</a>&gt;</p>\n";
    stream << "<br />\n";

    // TODO: possibly retrieve from git history at build time?
    //: %1 is the name of the launcher, determined at build time, e.g. "PolyMC Developers"
    stream << "<h3>" << QObject::tr("%1 Developers", "About Credits").arg("MultiMC") << "</h3>\n";
    stream << "<p>Andrew Okin &lt;<a href='mailto:forkk@forkk.net'>forkk@forkk.net</a>&gt;</p>\n";
    stream << "<p>Petr Mrázek &lt;<a href='mailto:peterix@gmail.com'>peterix@gmail.com</a>&gt;</p>\n";
    stream << "<p>Sky Welch &lt;<a href='mailto:multimc@bunnies.io'>multimc@bunnies.io</a>&gt;</p>\n";
    stream << "<p>Jan (02JanDal) &lt;<a href='mailto:02jandal@gmail.com'>02jandal@gmail.com</a>&gt;</p>\n";
    stream << "<p>RoboSky &lt;<a href='https://twitter.com/RoboSky_'>@RoboSky_</a>&gt;</p>\n";
    stream << "<br />\n";

    stream << "<h3>" << QObject::tr("With thanks to", "About Credits") << "</h3>\n";
    stream << "<p>Orochimarufan &lt;<a href='mailto:orochimarufan.x3@gmail.com'>orochimarufan.x3@gmail.com</a>&gt;</p>\n";
    stream << "<p>TakSuyu &lt;<a href='mailto:taksuyu@gmail.com'>taksuyu@gmail.com</a>&gt;</p>\n";
    stream << "<p>Kilobyte &lt;<a href='mailto:stiepen22@gmx.de'>stiepen22@gmx.de</a>&gt;</p>\n";
    stream << "<p>Rootbear75 &lt;<a href='https://twitter.com/rootbear75'>@rootbear75</a>&gt;</p>\n";
    stream << "<p>Zeker Zhayard &lt;<a href='https://twitter.com/zeker_zhayard'>@Zeker_Zhayard</a>&gt;</p>\n";
    stream << "<p>Everyone else who <a href='https://github.com/PolyMC/PolyMC/graphs/contributors'>contributed</a>!</p>\n";
    stream << "<br />\n";

    stream << "</center>\n";
    return output;
}

QString getLicenseHtml()
{
    HoeDown hoedown;
    QFile dataFile(":/documents/COPYING.md");
    dataFile.open(QIODevice::ReadOnly);
    QString output = hoedown.process(dataFile.readAll());
    return output;
}

}

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    QString launcherName = BuildConfig.LAUNCHER_NAME;

    setWindowTitle(tr("About %1").arg(launcherName));

    QString chtml = getCreditsHtml();
    ui->creditsText->setHtml(chtml);

    QString lhtml = getLicenseHtml();
    ui->licenseText->setHtml(lhtml);

    ui->urlLabel->setOpenExternalLinks(true);

    ui->icon->setPixmap(APPLICATION->getThemedIcon("logo").pixmap(64));
    ui->title->setText(launcherName);

    ui->versionLabel->setText(BuildConfig.printableVersionString());

    if (!BuildConfig.BUILD_PLATFORM.isEmpty())
        ui->platformLabel->setText(tr("Platform") +": " + BuildConfig.BUILD_PLATFORM);
    else
        ui->platformLabel->setVisible(false);

    if (BuildConfig.VERSION_BUILD >= 0)
        ui->buildNumLabel->setText(tr("Build Number") +": " + QString::number(BuildConfig.VERSION_BUILD));
    else
        ui->buildNumLabel->setVisible(false);

    if (!BuildConfig.VERSION_CHANNEL.isEmpty())
        ui->channelLabel->setText(tr("Channel") +": " + BuildConfig.VERSION_CHANNEL);
    else
        ui->channelLabel->setVisible(false);

    QString urlText("<html><head/><body><p><a href=\"%1\">%1</a></p></body></html>");
    ui->urlLabel->setText(urlText.arg(BuildConfig.LAUNCHER_GIT));

    QString copyText("© 2021-2022 %1");
    ui->copyLabel->setText(copyText.arg(BuildConfig.LAUNCHER_COPYRIGHT));

    connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

    connect(ui->aboutQt, &QPushButton::clicked, &QApplication::aboutQt);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
