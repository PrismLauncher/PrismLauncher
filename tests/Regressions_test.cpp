
#include <QDir>
#include <QStandardPaths>
#include <QTest>
#include <QUrl>

#include <filesystem>
#include <variant>
#include <vector>

#include <cli/Commands.h>
#include <cli/Parser.h>
#include <qtestcase.h>
#include <startup/Startup.h>
#include "BuildConfig.h"
#include "CLI/CLI.hpp"

namespace Cli {

// overload of QTest::toString
static char* toString(const Args& args)
{
    using namespace Cmd;
    struct {
        QString operator()(const Launch& launch)
        {
            struct {
                QString operator()(const Launch::NoTarget& /**/) { return "NoTarget{}"; }
                QString operator()(const Launch::ServerTarget& server)
                {
                    return QStringLiteral("ServerTarget{ target: \"%1\"}").arg(QString::fromStdString(server.target));
                }
                QString operator()(const Launch::WorldTarget& world)
                {
                    return QStringLiteral("WorldTarget{ target: \"%1\"}").arg(QString::fromStdString(world.target));
                }
            } targetVisitor;

            auto target = std::visit(targetVisitor, launch.target);

            struct {
                QString operator()(const Launch::AccountDefault& /**/) { return "AccountDefault{}"; }
                QString operator()(const Launch::AccountProfile& profile)
                {
                    return QStringLiteral("AccountProfile{ .name: \"%1\"}").arg(QString::fromStdString(profile.name));
                }
                QString operator()(const Launch::AccountOffline& profile)
                {
                    return QStringLiteral("AccountOffline{ name: \"%1\"}").arg(QString::fromStdString(profile.name));
                }
            } accountVisitor;

            auto account = std::visit(accountVisitor, launch.account);

            return QStringLiteral("Launch{ id: \"%1\", target: %2, account: %3 }").arg(QString::fromStdString(launch.id), target, account);
        }
        QString operator()(const ProcessURI& processURI)
        {
            return QStringLiteral("ProcessURI{ uri: \"%1\"}").arg(QString::fromStdString(processURI.uri));
        }
        QString operator()(const ShowMainWindow& /**/) { return "ShowMainWindow{}"; }
        QString operator()(const ShowInstanceWindow& showInstWin)
        {
            return QStringLiteral("ShowInstanceWindow{ id: \"%1\"}").arg(QString::fromStdString(showInstWin.id));
        }
        QString operator()(const Alive& alive)
        {
            return QStringLiteral("Alive{ path: \"%1\"}").arg(QString::fromStdU16String(alive.path.u16string()));
        }
    } cmdVisitor;

    QStringList cmds{};

    for (const auto& cmd : args.commands) {
        cmds.append(std::visit(cmdVisitor, cmd));
    }
    auto repr = QStringLiteral("Args{ commands: [%1]}").arg(cmds.join(", ")).toStdString();
    // QTest::toString requires us to output an alocated char*
    return qstrdup(repr.data());
}

}  // namespace Cli

class RegressionTests : public QObject {
    Q_OBJECT

    enum class CLIErrorType : std::uint16_t {
        Error,
        ParseError,
        OptionAlreadyAdded,
        CallForHelp,
        ArgumentMismatch,
        RequiresError,
        ExcludesError,
        FileError,
        ValidationError,
        OptionNotFound
    };

   private slots:

    // the exceptions we might expect to be thrown

    void test_legacyCli_data()
    {
        QTest::addColumn<std::vector<std::string>>("argv");
        QTest::addColumn<Cli::Args>("expected");
        QTest::addColumn<std::optional<CLIErrorType>>("throws");

        {
            using namespace Cli::Cmd;
            QTest::newRow("launch online")
                << std::vector<std::string>{ "--launch", "an_instance" }
                << Cli::Args{ .commands = {
                    Launch{ .id = "an_instance",
                        .target = Launch::NoTarget{},
                        .account = Launch::AccountDefault{}, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w show-window")
                << std::vector<std::string>{ "--launch", "an_instance", "--show-window" }
                << Cli::Args{ .commands = {
                    Launch{ .id = "an_instance",
                        .target = Launch::NoTarget{},
                        .account = Launch::AccountDefault{}, }, ShowMainWindow{}, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w show")
                << std::vector<std::string>{ "--launch", "an_instance", "--show", "an_instance" }
                << Cli::Args{ .commands = {
                    Launch{ .id = "an_instance",
                        .target = Launch::NoTarget{},
                        .account = Launch::AccountDefault{}, }, ShowInstanceWindow{ .id = "an_instance" }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online short")
                << std::vector<std::string>{ "-l", "an_instance" }
                << Cli::Args{ .commands = {
                                  Launch{ .id = "an_instance", .target = Launch::NoTarget{}, .account = Launch::AccountDefault{} }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("uri with launch")
                << std::vector<std::string>{ "a/uri", "-l", "an_instance" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::NoTarget{},
                                                    .account = Launch::AccountDefault{}, },
                                            ProcessURI{ .uri = "a/uri" }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w account")
                << std::vector<std::string>{ "--launch", "an_instance", "-a", "profile_name" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::NoTarget{},
                                                    .account = Launch::AccountProfile{ .name = "profile_name" }, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w server")
                << std::vector<std::string>{ "--launch", "an_instance", "-s", "mc.serveraddress.net:1234" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::ServerTarget{ .target = "mc.serveraddress.net:1234" },
                                                    .account = Launch::AccountDefault{}, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w world")
                << std::vector<std::string>{ "--launch", "an_instance", "-w", "world_name" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::WorldTarget{ .target = "world_name" },
                                                    .account = Launch::AccountDefault{}, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch online /w account /w server")
                << std::vector<std::string>{ "--launch", "an_instance", "-s", "mc.serveraddress.net:1234", "-a", "profile_name" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::ServerTarget{ .target = "mc.serveraddress.net:1234" },
                                                    .account = Launch::AccountProfile{ .name = "profile_name" }, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("launch offline")
                << std::vector<std::string>{ "--launch", "an_instance", "-o", "offline_name" }
                << Cli::Args{ .commands = { Launch{ .id = "an_instance",
                                                    .target = Launch::NoTarget{},
                                                    .account = Launch::AccountOffline{ .name = "offline_name" }, }, }, }
                << std::optional<CLIErrorType>{};
            QTest::newRow("cant launch offline with an account")
                << std::vector<std::string>{ "--launch", "an_instance", "-o", "offline_name", "-a", "profile_name" } << Cli::Args{}
                << std::optional<CLIErrorType>{ CLIErrorType::ExcludesError };
            QTest::newRow("cant launch with a world and server target")
                << std::vector<std::string>{ "--launch", "an_instance", "-w", "world_name", "-s", "mc.serveraddress.net:1234" }
                << Cli::Args{} << std::optional<CLIErrorType>{ CLIErrorType::ExcludesError };
            QTest::newRow("cant launch without --launch (world)") << std::vector<std::string>{ "-w", "world_name" } << Cli::Args{}
                                                                  << std::optional<CLIErrorType>{ CLIErrorType::RequiresError };
            QTest::newRow("cant launch without --launch (server)")
                << std::vector<std::string>{ "-s", "mc.serveraddress.net:1234" } << Cli::Args{}
                << std::optional<CLIErrorType>{ CLIErrorType::RequiresError };
            QTest::newRow("cant launch without --launch (account)") << std::vector<std::string>{ "-a", "profile_name" } << Cli::Args{}
                                                                    << std::optional<CLIErrorType>{ CLIErrorType::RequiresError };
            QTest::newRow("cant launch without --launch (ofline)") << std::vector<std::string>{ "-o", "offline_name" } << Cli::Args{}
                                                                   << std::optional<CLIErrorType>{ CLIErrorType::RequiresError };

            QTest::newRow("show main window") << std::vector<std::string>{ "--show-window" }
                                              << Cli::Args{ .commands = { ShowMainWindow{} } } << std::optional<CLIErrorType>{};
            QTest::newRow("show instance window")
                << std::vector<std::string>{ "--show", "an_instance" }
                << Cli::Args{ .commands = { ShowInstanceWindow{ .id = "an_instance" } } } << std::optional<CLIErrorType>{};

            QTest::newRow("alive") << std::vector<std::string>{ "--alive" }
                                   << Cli::Args{ .commands = { Alive{ .path = std::filesystem::current_path() } } }
                                   << std::optional<CLIErrorType>{};
            auto realPath = QFINDTESTDATA("testdata/Regressions");
            QTest::newRow("alive /w path") << std::vector<std::string>{ "--alive", realPath.toStdString() }
                                           << Cli::Args{ .commands = { Alive{ .path = realPath.toStdString() } } }
                                           << std::optional<CLIErrorType>{};
            QTest::newRow("alive /w bad path") << std::vector<std::string>{ "--alive", "path/to/loc" } << Cli::Args{}
                                               << std::optional<CLIErrorType>{ CLIErrorType::ValidationError };

            QTest::newRow("import") << std::vector<std::string>{ "-I", "path/to/import" }
                                    << Cli::Args{ .commands = { ProcessURI{ .uri = "path/to/import" } } } << std::optional<CLIErrorType>{};
            QTest::newRow("multi import") << std::vector<std::string>{ "-I", "path/to/import", "-I", "path/to/import2",
                                                                       "-I", "path/to/import3", }
                                          << Cli::Args{ .commands = { ProcessURI{ .uri = "path/to/import" },
                                                                      ProcessURI{ .uri = "path/to/import2" },
                                                                      ProcessURI{ .uri = "path/to/import3" }, }, }
                                          << std::optional<CLIErrorType>{};
            QTest::newRow("import after positional")
                << std::vector<std::string>{ "a_uri/path", "-I", "path/to/import" }
                << Cli::Args{ .commands = { ProcessURI{ .uri = "path/to/import" }, ProcessURI{ .uri = "a_uri/path" } } }
                << std::optional<CLIErrorType>{};
            QTest::newRow("import before positional")
                << std::vector<std::string>{ "-I", "path/to/import", "a_uri/path" }
                << Cli::Args{ .commands = { ProcessURI{ .uri = "path/to/import" }, ProcessURI{ .uri = "a_uri/path" } } }
                << std::optional<CLIErrorType>{};

            QTest::newRow("multi uri") << std::vector<std::string>{ "path/to/import",  "path/to/import2", "path/to/import3" }
                                       << Cli::Args{ .commands = { ProcessURI{ .uri = "path/to/import" },
                                                                   ProcessURI{ .uri = "path/to/import2" },
                                                                   ProcessURI{ .uri = "path/to/import3" }, }, }
                                       << std::optional<CLIErrorType>{};

            QTest::newRow("accepts a datapath")
                << std::vector<std::string>{ "-d", "~/prism" }
                << Cli::Args{ .dataPath = { .dataPath = "~/prism", .source = Startup::DataPathSource::Commandline }, .commands = {} }
                << std::optional<CLIErrorType>{};

            QTest::newRow("accepts a datapath with other commands")
                << std::vector<std::string>{ "-d",
                                             "~/prism",
                                             "-l",
                                             "an_instance",
                                             "-o",
                                             "offline_name",
                                             "-I",
                                             "path/to/import",
                                             "positional/import",
                                             "another/positional/import",
                                             "--show-window", }
                << Cli::Args{ .dataPath = { .dataPath = "~/prism", .source = Startup::DataPathSource::Commandline },
                              .commands = { Launch{
                                                .id = "an_instance",
                                                .target = Launch::NoTarget{},
                                                .account = Launch::AccountOffline{ .name = "offline_name" },

                                            },
                                            ShowMainWindow{},
                                            ProcessURI{ .uri = "path/to/import" },
                                            ProcessURI{ .uri = "positional/import" },
                                            ProcessURI{ .uri = "another/positional/import" },

                              }, }
                << std::optional<CLIErrorType>{};
        }
    }

    void test_legacyCli()
    {
        QFETCH(std::vector<std::string>, argv);
        QFETCH(Cli::Args, expected);
        QFETCH(std::optional<CLIErrorType>, throws);

        Cli::Args parsed{};
        CLI::App parser{};
        Cli::Cli cli{};
        cli.attach(parser, parsed);

        // not sure *WHY* these needs to be reversed but this is how CLI11's internal tests do it...
        // https://github.com/CLIUtils/CLI11/blob/eced4d837d0ab57242b1c25d03780d35bf2b0d52/tests/app_helper.hpp#L34
        std::ranges::reverse(argv);

        if (throws) {
            switch (throws.value()) {
                case CLIErrorType::ArgumentMismatch: {
                    QVERIFY_THROWS_EXCEPTION(CLI::ArgumentMismatch, parser.parse(argv));
                    break;
                }
                case CLIErrorType::OptionAlreadyAdded: {
                    QVERIFY_THROWS_EXCEPTION(CLI::OptionAlreadyAdded, parser.parse(argv));
                    break;
                }
                case CLIErrorType::CallForHelp: {
                    QVERIFY_THROWS_EXCEPTION(CLI::CallForHelp, parser.parse(argv));
                    break;
                }
                case CLIErrorType::RequiresError: {
                    QVERIFY_THROWS_EXCEPTION(CLI::RequiresError, parser.parse(argv));
                    break;
                }
                case CLIErrorType::ExcludesError: {
                    QVERIFY_THROWS_EXCEPTION(CLI::ExcludesError, parser.parse(argv));
                    break;
                }
                case CLIErrorType::ValidationError: {
                    QVERIFY_THROWS_EXCEPTION(CLI::ValidationError, parser.parse(argv));
                    break;
                }
                case CLIErrorType::FileError: {
                    QVERIFY_THROWS_EXCEPTION(CLI::FileError, parser.parse(argv));
                    break;
                }
                case CLIErrorType::OptionNotFound: {
                    QVERIFY_THROWS_EXCEPTION(CLI::OptionNotFound, parser.parse(argv));
                    break;
                }
                case CLIErrorType::ParseError: {
                    QVERIFY_THROWS_EXCEPTION(CLI::OptionNotFound, parser.parse(argv));
                    break;
                }
                case CLIErrorType::Error:
                default: {
                    QVERIFY_THROWS_EXCEPTION(CLI::Error, parser.parse(argv));
                }
            }
        } else {
            QVERIFY_THROWS_NO_EXCEPTION(parser.parse(argv));
        }

        QCOMPARE(parsed, expected);
    }

    void test_resolveStandardDataPath()
    {
        // get standard writable loc and peel off qt's auto added app name
        auto standardLocation =
            std::filesystem::path{ QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdU16String() }.parent_path();

        const auto rootDirs = QDir(QFINDTESTDATA("testdata/Regressions")).absolutePath();
        const auto nonPortableRoot = std::filesystem::path{ rootDirs.toStdU16String() } / "NonPortableRoot";

#ifndef Q_OS_MACOS
        const auto portableTxtRoot = std::filesystem::path{ rootDirs.toStdU16String() } / "PortableTXTRoot";
        const auto userDataRoot = std::filesystem::path{ rootDirs.toStdU16String() } / "UserDataRoot";

        const auto portableTxtResult = Startup::resolveDataPath(portableTxtRoot);
        QCOMPARE(portableTxtResult.dataPath, portableTxtRoot);

        const auto userDataResult = Startup::resolveDataPath(userDataRoot);
        QCOMPARE(userDataResult.dataPath, userDataRoot / "UserData");
#endif

        const auto nonPortableResult = Startup::resolveDataPath(nonPortableRoot);
        const auto nonPortableDataPath = QString::fromStdU16String(nonPortableResult.dataPath.u16string());
        QVERIFY2(standardLocation == nonPortableResult.dataPath.parent_path(),
                 qPrintable(QString("non-portable dataPath `%1` is not in the standard location").arg(nonPortableDataPath)));
        QCOMPARE(QString::fromStdU16String(nonPortableResult.dataPath.filename().u16string()), BuildConfig.LAUNCHER_NAME);
    }
};

QTEST_GUILESS_MAIN(RegressionTests)
#include "Regressions_test.moc"
