#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/NetworkHttpClient.h>
#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

#include <memory>

namespace {

// Keeps the requests until the test answers them
struct FakeClient : PrismApi::HttpClient {
    struct Pending {
        PrismApi::HttpRequest request;
        std::function<void(const PrismApi::HttpResponse&)> done;
    };
    void send(const PrismApi::HttpRequest& request, std::function<void(const PrismApi::HttpResponse&)> done) override
    {
        pending.append({ request, std::move(done) });
    }
    void answer(const PrismApi::HttpResponse& response)
    {
        auto next = pending.takeFirst();
        next.done(response);
    }
    QList<Pending> pending;
};

// Answers each request with the response for its path, then closes the connection
class TinyServer : public QTcpServer {
   public:
    QHash<QByteArray, QByteArray> responses;

   protected:
    void incomingConnection(qintptr handle) override
    {
        auto* socket = new QTcpSocket(this);
        socket->setSocketDescriptor(handle);
        connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
            m_buffers[socket] += socket->readAll();
            const QByteArray& data = m_buffers[socket];
            if (!data.contains("\r\n\r\n")) {
                return;
            }
            const QByteArray path = data.split(' ').value(1);
            socket->write(responses.value(path, "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\nnot found"));
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }

   private:
    QHash<QTcpSocket*, QByteArray> m_buffers;
};

}  // namespace

class HttpApiTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    FakeClient m_client;

    std::unique_ptr<LuaState> newState(const QStringList& permissions = { "net" }, PrismApi::HttpClient* client = nullptr)
    {
        PluginManifest manifest;
        manifest.id = "com.example.http";
        manifest.permissions = permissions;
        auto state = std::make_unique<LuaState>();
        if (auto error = PrismApi::install(*state, manifest, m_dir.path(), { .http = client ? client : &m_client })) {
            qFatal("install failed: %s", qPrintable(*error));
        }
        // moc does not understand raw strings, and a "//" inside one breaks it: the scripts below build URLs with this
        state->run("function url(scheme, rest) return scheme .. '://' .. rest end", "helpers");
        return state;
    }

    static void runOk(LuaState& state, const QByteArray& code)
    {
        auto error = state.run(code, "test");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    static void expectError(LuaState& state, const QByteArray& code, const QString& expected)
    {
        auto error = state.run(code, "test");
        QVERIFY2(error, "the code should have failed");
        QVERIFY2(error->contains(expected), qPrintable(*error));
    }

   private slots:
    void cleanup() { m_client.pending.clear(); }

    void test_getWaitsForTheResponse()
    {
        auto state = newState();
        runOk(*state, R"lua(
            prism.async(function()
                stage = "waiting"
                local r = prism.http.get(url("https", "example.com/data?x=1"), { headers = { Accept = "text/plain" }, timeout = 5 })
                stage = "done"
                result = r
            end)
        )lua");
        runOk(*state, "assert(stage == 'waiting')");
        QCOMPARE(m_client.pending.size(), 1);
        const auto& request = m_client.pending.first().request;
        QCOMPARE(request.method, "GET");
        QCOMPARE(request.url, QUrl("https://example.com/data?x=1"));
        QCOMPARE(request.headers.size(), 1);
        QCOMPARE(request.headers.first().first, "Accept");
        QCOMPARE(request.timeoutMs, 5000);

        m_client.answer({ 200, "hello", { { "content-type", "text/plain" } }, {} });
        runOk(*state, R"lua(
            assert(stage == "done")
            assert(result.status == 200 and result.ok == true and result.body == "hello", result.body)
            assert(result.headers["content-type"] == "text/plain")
            assert(result.error == nil)
        )lua");
    }

    void test_postAndSequentialRequests()
    {
        auto state = newState();
        runOk(*state, R"lua(
            prism.async(function()
                local first = prism.http.post(url("http", "localhost/api"), '{"a":1}')
                local second = prism.http.get(url("http", "localhost/after/") .. first.status)
                statuses = first.status .. "," .. second.status
            end)
        )lua");
        QCOMPARE(m_client.pending.first().request.method, "POST");
        QCOMPARE(m_client.pending.first().request.body, "{\"a\":1}");
        m_client.answer({ 201, {}, {}, {} });
        QCOMPARE(m_client.pending.first().request.url, QUrl("http://localhost/after/201"));
        m_client.answer({ 404, "missing", {}, {} });
        runOk(*state, "assert(statuses == '201,404', statuses)");
    }

    void test_failures()
    {
        auto state = newState();
        runOk(*state, R"lua(
            prism.async(function()
                local r = prism.http.get(url("https", "example.invalid/"))
                failure = r
            end)
        )lua");
        m_client.answer({ 0, {}, {}, "Host not found" });
        runOk(*state, "assert(failure.ok == false and failure.status == 0 and failure.error == 'Host not found')");
    }

    void test_onlyInsideTasks()
    {
        auto state = newState();
        expectError(*state, "prism.http.get('https://example.com')", "must be called from a task started with prism.async");
        expectError(*state, "coroutine.wrap(function() prism.http.get('https://example.com') end)()",
                    "must be called from a task started with prism.async");
        QVERIFY(m_client.pending.isEmpty());
    }

    void test_invalidRequests()
    {
        auto state = newState();
        // Plain strings: moc misreads raw strings with these quotes
        runOk(*state,
              "function fails(fn, expected)\n"
              "    prism.async(function()\n"
              "        local ok, err = pcall(fn)\n"
              "        assert(not ok and err:find(expected, 1, true), tostring(err))\n"
              "        checked = (checked or 0) + 1\n"
              "    end)\n"
              "end");
        const QByteArray site = "url('https', 'example.com')";
        const QList<QByteArray> cases = {
            "fails(function() prism.http.get(url('ftp', 'example.com/file')) end, 'is not an http or https URL')",
            "fails(function() prism.http.get('not a url') end, 'is not an http or https URL')",
            "fails(function() prism.http.get(" + site + ", { headers = { ['X-A'] = '1\\r\\nX-B: 2' } }) end, 'invalid header')",
            "fails(function() prism.http.get(" + site + ", { headers = { 1 } }) end, 'headers must map names to strings')",
            "fails(function() prism.http.get(" + site + ", { timeout = 0 }) end, 'must be between 0 and')",
            "fails(function() prism.http.get(" + site + ", { timeout = 1000 }) end, 'must be between 0 and')",
            "fails(function() prism.http.post(" + site + ") end, 'bad argument #2')",
        };
        for (const QByteArray& code : cases) {
            runOk(*state, code);
        }
        runOk(*state, "assert(checked == 7, checked)");
        QVERIFY(m_client.pending.isEmpty());
    }

    void test_needsPermission()
    {
        auto state = newState({});
        // Errors inside a task are logged, not raised to the caller
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("async task failed: .*prism.http requires the 'net' permission"));
        runOk(*state, "prism.async(function() prism.http.get('https://example.com') end)");
    }

    void test_pendingLimit()
    {
        auto state = newState();
        runOk(*state, QString(R"lua(
            for i = 1, %1 do
                prism.async(function() prism.http.get(url("https", "example.com/") .. i) end)
            end
            prism.async(function()
                local ok, err = pcall(prism.http.get, url("https", "example.com/extra"))
                limited = not ok and err:find("too many requests at once", 1, true) ~= nil
            end)
        )lua")
                          .arg(PrismApi::s_maxPendingHttpRequests)
                          .toUtf8());
        runOk(*state, "assert(limited)");
        QCOMPARE(m_client.pending.size(), PrismApi::s_maxPendingHttpRequests);

        // Once one finishes there is room again
        m_client.answer({ 200, {}, {}, {} });
        runOk(*state, "prism.async(function() prism.http.get('https://example.com/again') end)");
        QCOMPARE(m_client.pending.size(), PrismApi::s_maxPendingHttpRequests);
    }

    void test_freshBudgetAfterWaiting()
    {
        auto state = newState();
        state->setInstructionLimit(200'000);
        runOk(*state, R"lua(
            prism.async(function()
                for i = 1, 30000 do end
                prism.http.get(url("https", "example.com"))
                for i = 1, 30000 do end
                finished = true
            end)
        )lua");
        m_client.answer({ 200, {}, {}, {} });
        runOk(*state, "assert(finished)");
    }

    void test_errorsInTasksAreLogged()
    {
        auto state = newState();
        runOk(*state, "prism.async(function() prism.http.get('https://example.com') error('after waiting') end)");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^\\[com.example.http\\] async task failed: test:1: after waiting"));
        m_client.answer({ 200, {}, {}, {} });

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^\\[com.example.http\\] async task failed: test:1: right away"));
        runOk(*state, "prism.async(function() error('right away') end)");
        runOk(*state, "prism.async(function(a, b) sum = a + b end, 2, 3) assert(sum == 5)");
    }

    void test_unloadedWhileWaiting()
    {
        auto state = newState();
        runOk(*state, "prism.async(function() prism.http.get('https://example.com') reached = true end)");
        state.reset();
        // The plugin is gone; answering must not touch it
        m_client.answer({ 200, {}, {}, {} });
    }

    void test_networkClient()
    {
        TinyServer server;
        server.responses["/hello"] =
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nX-Custom: yes\r\nContent-Length: 5\r\nConnection: close\r\n\r\nhello";
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const QString base = QString("http://127.0.0.1:%1").arg(server.serverPort());

        QNetworkAccessManager network;
        NetworkHttpClient client(&network, "PrismLauncher-test");
        auto state = newState({ "net" }, &client);
        runOk(*state, QString(R"lua(
            prism.async(function()
                hello = prism.http.get("%1/hello")
                missing = prism.http.get("%1/missing")
            end)
        )lua")
                          .arg(base)
                          .toUtf8());
        QVERIFY(QTest::qWaitFor([&] { return !state->run("assert(missing)", "check"); }, 10'000));
        runOk(*state, R"lua(
            assert(hello.ok and hello.status == 200 and hello.body == "hello", hello.body)
            assert(hello.headers["content-type"] == "text/plain" and hello.headers["x-custom"] == "yes")
            assert(not missing.ok and missing.status == 404 and missing.body == "not found" and missing.error == nil)
        )lua");

        // Nothing listens on this port
        server.close();
        runOk(*state, QString("prism.async(function() refused = prism.http.get('%1/hello') end)").arg(base).toUtf8());
        QVERIFY(QTest::qWaitFor([&] { return !state->run("assert(refused)", "check"); }, 10'000));
        runOk(*state, "assert(refused.ok == false and refused.status == 0 and #refused.error > 0, refused.error)");
    }
};

QTEST_GUILESS_MAIN(HttpApiTest)

#include "HttpApi_test.moc"
