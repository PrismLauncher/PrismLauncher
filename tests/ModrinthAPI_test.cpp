// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QTest>

#include <QStringList>

#include <modplatform/modrinth/ModrinthAPI.h>

class ModrinthAPITest : public QObject {
    Q_OBJECT

   private slots:

    void searchFacets_data()
    {
        QTest::addColumn<QStringList>("categories");
        QTest::addColumn<ModPlatform::ModLoaderTypes>("loaders");
        QTest::addColumn<QString>("facets");

        // Modrinth ANDs separate arrays and ORs the entries of one array, so each category needs an
        // array of its own for the search to narrow down as more of them are ticked.
        QTest::newRow("no categories") << QStringList{} << ModPlatform::ModLoaderTypes{} << R"([["project_type:mod"]])";
        QTest::newRow("one category") << QStringList{ "adventure" } << ModPlatform::ModLoaderTypes{}
                                      << R"([["categories:adventure"],["project_type:mod"]])";
        QTest::newRow("two categories") << QStringList{ "adventure", "magic" } << ModPlatform::ModLoaderTypes{}
                                        << R"([["categories:adventure"],["categories:magic"],["project_type:mod"]])";

        // Loaders share one array on purpose: a mod is built for one of them, so they have to be ORed.
        QTest::newRow("two loaders") << QStringList{} << ModPlatform::ModLoaderTypes(ModPlatform::Fabric | ModPlatform::Quilt)
                                     << R"([["categories:fabric","categories:quilt"],["project_type:mod"]])";
    }

    void searchFacets()
    {
        QFETCH(QStringList, categories);
        QFETCH(ModPlatform::ModLoaderTypes, loaders);
        QFETCH(QString, facets);

        ResourceAPI::SearchArgs args{};
        args.type = ModPlatform::ResourceType::Mod;
        if (!categories.isEmpty()) {
            args.categoryIds = categories;
        }
        if (loaders != 0) {
            args.loaders = loaders;
        }

        ModrinthAPI api;
        auto url = api.getSearchURL(args);

        QVERIFY(url.has_value());
        QCOMPARE(url->section("facets=", 1), facets);
    }
};

QTEST_GUILESS_MAIN(ModrinthAPITest)

#include "ModrinthAPI_test.moc"
