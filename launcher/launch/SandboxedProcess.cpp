#include <modrinth_sandbox.h>
#include <QScopeGuard>
#include <array>

#include "Defer.h"
#include "Handle.h"
#include "SandboxedProcess.h"

SandboxedProcess::SandboxedProcess(QStringConverter::Encoding encoding) : m_encoding(encoding) {}

SandboxedProcess::~SandboxedProcess()
{
    kill();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void SandboxedProcess::start(Options opts)
{
    m_thread = std::thread([this, opts = std::move(opts)] { exec(opts); });
}

void SandboxedProcess::kill()
{
    std::scoped_lock lock(m_stateLock);
    if (m_child == nullptr) {
        return;
    }

    modrinth_sandbox_child_kill(m_child);
}

bool SandboxedProcess::write(const QString& data)
{
    QStringEncoder encoder(m_encoding);
    const QByteArray bytes = encoder(data);

    std::scoped_lock lock(m_stateLock);
    if (m_stdin == nullptr) {
        return false;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) needed due to API limitations
    const auto* uintData = reinterpret_cast<const std::uint8_t*>(bytes.data());
    return modrinth_sandbox_pipe_writer_write(m_stdin, uintData, bytes.size(), nullptr);
}

namespace {
ModrinthSandboxString makeSandboxStr(std::string_view str)
{
    return { .ptr = str.data(), .len = str.size() };
}

ModrinthSandboxStringOption makeSandboxStrOption(std::string_view str)
{
    return { .ptr = str.data(), .len = str.size() };
}

QString sandboxLastError()
{
    const char* msg = nullptr;
    std::size_t len = 0;
    modrinth_sandbox_get_last_error(&msg, &len);
    return QString::fromUtf8(msg, static_cast<qsizetype>(len));
}

// holds ownership of slices provided in ModrinthSandboxCommandView
struct SandboxCommandBackingData {
    std::vector<ModrinthSandboxString> args;
    std::vector<ModrinthSandboxString> envKeys;
    std::vector<ModrinthSandboxString> envValues;
    std::vector<ModrinthSandboxString> readOnlyPaths;
    std::vector<ModrinthSandboxString> readWritePaths;
};

std::pair<ModrinthSandboxCommandView, SandboxCommandBackingData> makeSandboxCommand(const SandboxedProcess::Options& opts)
{
    SandboxCommandBackingData d{};
    d.args.reserve(opts.args.size());
    for (const auto& arg : opts.args) {
        d.args.push_back(makeSandboxStr(arg));
    }

    d.envKeys.reserve(opts.env.size());
    for (const auto& pair : opts.env) {
        d.envKeys.push_back(makeSandboxStr(pair.first));
        d.envValues.push_back(makeSandboxStr(pair.second));
    }

    ModrinthSandboxCommandView result{
        .executable = makeSandboxStr(opts.command),
        .args = { .ptr = d.args.data(), .len = d.args.size() },
        .ensure_dirs_exist = {},
        .read_only_paths = {},
        .read_write_paths = {},
        .working_directory = makeSandboxStrOption(opts.workingDir),
        .passthrough_environment = { },
        .extra_environment = {
            .key_ptr = d.envKeys.data(),
            .key_len = d.envKeys.size(),
            .value_ptr = d.envValues.data(),
            .value_len = d.envValues.size(),
        },
        .allow_network = false,
        .is_jvm = false,
        .die_with_parent = true,
        .child_stdin = MODRINTH_SANDBOX_STDIO_PIPE,
        .child_stdout = MODRINTH_SANDBOX_STDIO_PIPE,
        .child_stderr = MODRINTH_SANDBOX_STDIO_PIPE,
        // the type indicates these should not be null... even though we don't have anything useful to put here with sandboxing off
        .app_container_name = makeSandboxStr(""),
        .app_container_description = makeSandboxStr(""),
    };

    if (opts.sandbox.has_value()) {
        const auto& sandbox = opts.sandbox.value();
        result.allow_network = sandbox.allowNetwork;
        result.is_jvm = sandbox.isJvm;
        result.app_container_name = makeSandboxStr(sandbox.appContainerName);
        result.app_container_description = makeSandboxStr(sandbox.appContainerDescription);

        d.readOnlyPaths.reserve(sandbox.readOnlyPaths.size());
        for (const auto& path : sandbox.readOnlyPaths) {
            d.readOnlyPaths.push_back(makeSandboxStr(path));
        }
        result.read_only_paths = { .ptr = d.readOnlyPaths.data(), .len = d.readOnlyPaths.size() };

        d.readWritePaths.reserve(sandbox.readWritePaths.size());
        for (const auto& path : sandbox.readWritePaths) {
            d.readWritePaths.push_back(makeSandboxStr(path));
        }
        result.read_write_paths = { .ptr = d.readWritePaths.data(), .len = d.readWritePaths.size() };
    }

    return { result, std::move(d) };
}
}  // namespace

void SandboxedProcess::exec(const Options& opts)
{
    Handle<ModrinthSandboxEnv, modrinth_sandbox_env_free> env;
    const bool createEnvStatus =
        opts.sandbox.has_value() ? modrinth_sandbox_create_env(std::out_ptr(env)) : modrinth_sandbox_create_noop_env(std::out_ptr(env));
    if (!createEnvStatus) {
        emit failedToStart(sandboxLastError());
        return;
    }

    auto [cmdView, cmdBackingData] = makeSandboxCommand(opts);

    Handle<ModrinthSandboxCommand, modrinth_sandbox_command_free> cmd;
    if (!modrinth_sandbox_prepare_command(cmdView, std::out_ptr(cmd))) {
        emit failedToStart(sandboxLastError());
        return;
    }

    Handle<ModrinthSandboxChild, modrinth_sandbox_child_free> child;
    if (!modrinth_sandbox_spawn(env, std::inout_ptr(cmd), std::out_ptr(child))) {
        emit failedToStart("Failed to spawn child process: " + sandboxLastError());
        return;
    }

    std::uint32_t pid = 0;
    if (!modrinth_sandbox_child_id(child.get(), &pid)) {
        emit failedToStart("Failed to get process ID: " + sandboxLastError());
        modrinth_sandbox_child_kill(child);
        return;
    }
    emit started(pid);

    Handle<ModrinthSandboxPipeWriter, modrinth_sandbox_pipe_writer_free> stdinWriter;
    if (!modrinth_sandbox_child_take_stdin(child, std::out_ptr(stdinWriter))) {
        emit failedToStart("Failed to retrieve process stdin: " + sandboxLastError());
        return;
    }

    {
        std::scoped_lock lock(m_stateLock);
        m_child = child;
        m_stdin = stdinWriter;
    }
    DEFER([&] {
        std::scoped_lock lock(m_stateLock);
        m_child = nullptr;
        m_stdin = nullptr;
    });

    Handle<ModrinthSandboxPipeReader, modrinth_sandbox_pipe_reader_free> stdoutReader;
    if (!modrinth_sandbox_child_take_stdout(child, std::out_ptr(stdoutReader))) {
        emit failedToStart("Failed to retrieve process stdout: " + sandboxLastError());
        return;
    }

    Handle<ModrinthSandboxPipeReader, modrinth_sandbox_pipe_reader_free> stderrReader = nullptr;
    if (!modrinth_sandbox_child_take_stderr(child, std::out_ptr(stderrReader))) {
        emit failedToStart("Failed to retrieve process stderr: " + sandboxLastError());
        return;
    }

    std::thread stdoutReadLoop([this, stdoutReader = stdoutReader.get()] {
        execReadLoop(stdoutReader, [this](QStringList lines) { emit stdoutRead(std::move(lines)); });
    });
    DEFER([&] { stdoutReadLoop.join(); });
    std::thread stderrReadLoop([this, stderrReader = stderrReader.get()] {
        execReadLoop(stderrReader, [this](QStringList lines) { emit stderrRead(std::move(lines)); });
    });
    DEFER([&] { stderrReadLoop.join(); });

    ModrinthSandboxExitStatus status{};
    if (!modrinth_sandbox_child_wait(child, &status)) {
        emit failedToStart("Failed waiting for child: " + sandboxLastError());
        modrinth_sandbox_child_kill(child);
        return;
    }

    if (status.has_code) {
        emit finished(status.code);
    } else {
        emit finished(std::nullopt);
    }
}

template <typename TOutput>
void SandboxedProcess::execReadLoop(ModrinthSandboxPipeReader* reader, TOutput output)
{
    std::array<std::uint8_t, 8192> buf{};

    std::size_t readCount = 0;
    QStringDecoder decoder(m_encoding);
    QString unprocessed;
    while (true) {
        const bool status = modrinth_sandbox_pipe_reader_read(reader, buf.data(), buf.size(), &readCount);
        if (!status) {
            output(QStringList{ tr("---- Stopped reading output - %1 ----").arg(sandboxLastError()) });
            return;
        }

        if (readCount == 0) {
            break;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) needed due to API limitations
        const QByteArrayView view(reinterpret_cast<const char*>(buf.data()), static_cast<qsizetype>(readCount));
        QString str = unprocessed + decoder(view);
        str.remove('\r');
        QStringList lines = str.split('\n');
        unprocessed = lines.takeLast();

        output(std::move(lines));
    }
}
