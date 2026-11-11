#pragma once

#include <QString>
#include <QStringConverter>
#include <QStringList>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct ModrinthSandboxChild;
struct ModrinthSandboxPipeReader;
struct ModrinthSandboxPipeWriter;

/// SandboxedProcess allow launching and watching a process that may have sandboxing enabled using the modrinth-sandbox library.
class SandboxedProcess : public QObject {
    Q_OBJECT
   public:
    SandboxedProcess(QStringConverter::Encoding encoding);

    ~SandboxedProcess() override;

    struct SandboxOptions {
        bool isJvm;
        bool allowNetwork;
        std::string appContainerName;
        std::string_view appContainerDescription;
        std::vector<std::string> readOnlyPaths;
        std::vector<std::string> readWritePaths;
    };

    struct Options {
        std::string command;
        std::vector<std::string> args;
        std::string workingDir;
        std::vector<std::pair<std::string, std::string>> env;
        std::optional<SandboxOptions> sandbox;
    };

    void start(Options opts);

    void kill();

    [[nodiscard]] bool write(const QString& data);

   signals:
    void started(std::uint32_t pid);

    void finished(std::optional<std::int32_t> exitCode);

    void failedToStart(QString errorMessage);

    void stdoutRead(QStringList lines);

    void stderrRead(QStringList lines);

   private:
    void exec(const Options& opts);

    template <typename TOutput>
    void execReadLoop(ModrinthSandboxPipeReader* reader, TOutput output);

    const QStringConverter::Encoding m_encoding;

    std::thread m_thread;

    std::mutex m_stateLock;
    ModrinthSandboxChild* m_child = nullptr;
    ModrinthSandboxPipeWriter* m_stdin = nullptr;
};
