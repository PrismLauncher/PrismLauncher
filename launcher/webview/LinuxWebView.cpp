// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

// WebKitGTK 4.1 (GTK 3) implementation of webview::WebView.
//
// Event loop: Qt on Linux uses a GLib based event dispatcher, which iterates the default GMainContext
// that GTK and WebKit also use, so both toolkits run on the same (GUI) thread without a second loop.
// If Qt was built without GLib support we fall back to pumping the GLib context from a Qt timer.

#include "WebView.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

// Qt defines `signals`, `slots` and `emit` as macros; GLib/GIO headers use `signals` as an identifier.
#pragma push_macro("signals")
#pragma push_macro("slots")
#pragma push_macro("emit")
#undef signals
#undef slots
#undef emit
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <webkit2/webkit2.h>
#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")

namespace webview {
namespace {

constexpr auto MessageHandlerName = "materialmc";

bool ensureGtk(QString* error)
{
    static bool s_initialized = false;
    static bool s_ok = false;
    if (s_initialized) {
        if (!s_ok && error) {
            *error = QStringLiteral("GTK could not be initialised (no display?)");
        }
        return s_ok;
    }
    s_initialized = true;
    s_ok = gtk_init_check(nullptr, nullptr) != FALSE;
    if (!s_ok) {
        if (error) {
            *error = QStringLiteral("GTK could not be initialised (no display?)");
        }
        return false;
    }

    auto* dispatcher = QAbstractEventDispatcher::instance();
    const QByteArray dispatcherName = dispatcher ? QByteArray(dispatcher->metaObject()->className()) : QByteArray();
    if (!dispatcherName.contains("Glib")) {
        qWarning() << "WebView: Qt event dispatcher" << dispatcherName << "is not GLib based, pumping the GLib main context manually";
        auto* pump = new QTimer(QCoreApplication::instance());
        pump->setInterval(8);
        QObject::connect(pump, &QTimer::timeout, pump, [] {
            while (g_main_context_iteration(nullptr, FALSE)) {
            }
        });
        pump->start();
    }
    return true;
}

class LinuxWebView final : public WebView {
   public:
    explicit LinuxWebView(Options options) : m_options(std::move(options)) {}

    ~LinuxWebView() override
    {
        if (m_window) {
            g_signal_handlers_disconnect_by_data(m_window, this);
            if (m_contentManager) {
                g_signal_handlers_disconnect_by_data(m_contentManager, this);
            }
            if (m_view) {
                g_signal_handlers_disconnect_by_data(m_view, this);
            }
            gtk_widget_destroy(m_window);
        }
        if (m_contentManager) {
            g_object_unref(m_contentManager);
        }
        if (m_context) {
            g_object_unref(m_context);
        }
    }

    bool init(QString* error)
    {
        m_context = webkit_web_context_new_ephemeral();
        webkit_web_context_register_uri_scheme(m_context, AppScheme, &LinuxWebView::onSchemeRequest, this, nullptr);
        auto* security = webkit_web_context_get_security_manager(m_context);
        webkit_security_manager_register_uri_scheme_as_secure(security, AppScheme);
        webkit_security_manager_register_uri_scheme_as_cors_enabled(security, AppScheme);

        m_contentManager = webkit_user_content_manager_new();
        const auto script = bootstrapScript(QByteArrayLiteral("window.webkit.messageHandlers.materialmc.postMessage(s);"));
        auto* userScript = webkit_user_script_new(script.constData(), WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
                                                  WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, nullptr, nullptr);
        webkit_user_content_manager_add_script(m_contentManager, userScript);
        webkit_user_script_unref(userScript);
        g_signal_connect(m_contentManager, "script-message-received::materialmc", G_CALLBACK(&LinuxWebView::onScriptMessage), this);
        if (!webkit_user_content_manager_register_script_message_handler(m_contentManager, MessageHandlerName)) {
            if (error) {
                *error = QStringLiteral("Could not register the WebKit script message handler");
            }
            return false;
        }

        // The view adds its own references to the context and the content manager; ours are dropped in the destructor.
        m_view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, "web-context", m_context, "user-content-manager", m_contentManager,
                                              nullptr));

        auto* settings = webkit_web_view_get_settings(m_view);
        webkit_settings_set_enable_developer_extras(settings, m_options.developerTools);
        webkit_settings_set_javascript_can_open_windows_automatically(settings, FALSE);
        webkit_settings_set_javascript_can_access_clipboard(settings, FALSE);
        webkit_settings_set_allow_file_access_from_file_urls(settings, FALSE);
        webkit_settings_set_allow_universal_access_from_file_urls(settings, FALSE);
        webkit_settings_set_enable_back_forward_navigation_gestures(settings, FALSE);
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, m_options.developerTools);
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);

        GdkRGBA background{ 0.067, 0.078, 0.094, 1.0 };  // matches --bg, avoids a white flash
        webkit_web_view_set_background_color(m_view, &background);

        g_signal_connect(m_view, "decide-policy", G_CALLBACK(&LinuxWebView::onDecidePolicy), this);
        g_signal_connect(m_view, "create", G_CALLBACK(&LinuxWebView::onCreate), this);
        g_signal_connect(m_view, "context-menu", G_CALLBACK(&LinuxWebView::onContextMenu), this);
        g_signal_connect(m_view, "permission-request", G_CALLBACK(&LinuxWebView::onPermissionRequest), this);
        g_signal_connect(m_view, "web-process-terminated", G_CALLBACK(&LinuxWebView::onWebProcessTerminated), this);

        m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(m_window), m_options.title.toUtf8().constData());
        gtk_window_set_default_size(GTK_WINDOW(m_window), m_options.size.width(), m_options.size.height());
        GdkGeometry hints{};
        hints.min_width = m_options.minimumSize.width();
        hints.min_height = m_options.minimumSize.height();
        gtk_window_set_geometry_hints(GTK_WINDOW(m_window), nullptr, &hints, GDK_HINT_MIN_SIZE);
        if (!m_options.iconName.isEmpty()) {
            gtk_window_set_icon_name(GTK_WINDOW(m_window), m_options.iconName.toUtf8().constData());
        }
        gtk_container_add(GTK_CONTAINER(m_window), GTK_WIDGET(m_view));
        g_signal_connect(m_window, "delete-event", G_CALLBACK(&LinuxWebView::onDeleteEvent), this);
        return true;
    }

    QString engineName() const override
    {
        return QStringLiteral("WebKitGTK %1.%2.%3").arg(webkit_get_major_version()).arg(webkit_get_minor_version()).arg(webkit_get_micro_version());
    }

    void load(const QUrl& url) override { webkit_web_view_load_uri(m_view, url.toString(QUrl::FullyEncoded).toUtf8().constData()); }

    void show() override { gtk_widget_show_all(m_window); }
    void hide() override { gtk_widget_hide(m_window); }
    void present() override
    {
        gtk_widget_show_all(m_window);
        gtk_window_present(GTK_WINDOW(m_window));
    }
    bool isVisible() const override { return gtk_widget_get_visible(m_window) != FALSE; }
    void setTitle(const QString& title) override { gtk_window_set_title(GTK_WINDOW(m_window), title.toUtf8().constData()); }

    void postMessage(const QByteArray& json) override
    {
        const auto script = deliverScript(json);
        webkit_web_view_evaluate_javascript(m_view, script.constData(), script.size(), nullptr, nullptr, nullptr, nullptr, nullptr);
    }

   private:
    bool isTrusted(const char* uri) const { return uri && isTrustedUrl(QUrl(QString::fromUtf8(uri)), m_options.devOrigin); }

    static void onScriptMessage(WebKitUserContentManager*, WebKitJavascriptResult* result, gpointer data)
    {
        auto* self = static_cast<LinuxWebView*>(data);
        if (!self->isTrusted(webkit_web_view_get_uri(self->m_view))) {
            qWarning() << "WebView: dropped bridge message from untrusted page" << webkit_web_view_get_uri(self->m_view);
            return;
        }
        JSCValue* value = webkit_javascript_result_get_js_value(result);
        if (!jsc_value_is_string(value)) {
            return;
        }
        gchar* text = jsc_value_to_string(value);
        const QByteArray message(text);
        g_free(text);
        if (self->m_messageHandler) {
            self->m_messageHandler(message);
        }
    }

    static void onSchemeRequest(WebKitURISchemeRequest* request, gpointer data)
    {
        auto* self = static_cast<LinuxWebView*>(data);
        const QUrl url(QString::fromUtf8(webkit_uri_scheme_request_get_uri(request)));

        Response response = Response::notFound();
        if (url.host() != QLatin1String(AppHost)) {
            response = Response::forbidden();
        } else if (self->m_resourceHandler) {
            response = self->m_resourceHandler(url.path(QUrl::FullyDecoded));
        }
        if (response.mimeType.isEmpty()) {
            response.mimeType = mimeTypeFor(url.path());
        }

        // GBytes copies the data, so the response body can go out of scope.
        GBytes* bytes = g_bytes_new(response.body.constData(), static_cast<gsize>(response.body.size()));
        GInputStream* stream = g_memory_input_stream_new_from_bytes(bytes);
        g_bytes_unref(bytes);

        WebKitURISchemeResponse* schemeResponse = webkit_uri_scheme_response_new(stream, response.body.size());
        webkit_uri_scheme_response_set_status(schemeResponse, static_cast<guint>(response.status), nullptr);
        webkit_uri_scheme_response_set_content_type(schemeResponse, response.mimeType.constData());

        SoupMessageHeaders* headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        soup_message_headers_append(headers, "Content-Security-Policy", contentSecurityPolicy().constData());
        soup_message_headers_append(headers, "X-Content-Type-Options", "nosniff");
        soup_message_headers_append(headers, "Cache-Control", "no-store");
        webkit_uri_scheme_response_set_http_headers(schemeResponse, headers);  // takes ownership

        webkit_uri_scheme_request_finish_with_response(request, schemeResponse);
        g_object_unref(schemeResponse);
        g_object_unref(stream);
    }

    static gboolean onDecidePolicy(WebKitWebView*, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, gpointer data)
    {
        auto* self = static_cast<LinuxWebView*>(data);
        if (type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE) {
            return FALSE;  // default handling
        }
        auto* navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction* action = webkit_navigation_policy_decision_get_navigation_action(navigation);
        WebKitURIRequest* request = webkit_navigation_action_get_request(action);
        const char* uri = webkit_uri_request_get_uri(request);

        if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && self->isTrusted(uri)) {
            webkit_policy_decision_use(decision);
            return TRUE;
        }
        // Everything else (other origins, new windows) never loads inside the app.
        webkit_policy_decision_ignore(decision);
        const QUrl url(QString::fromUtf8(uri ? uri : ""));
        const bool userInitiated = webkit_navigation_action_is_user_gesture(action) != FALSE;
        if (userInitiated && (url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("http")) &&
            self->m_externalUrlHandler) {
            self->m_externalUrlHandler(url);
        }
        return TRUE;
    }

    static GtkWidget* onCreate(WebKitWebView*, WebKitNavigationAction*, gpointer) { return nullptr; }

    static gboolean onContextMenu(WebKitWebView*, WebKitContextMenu*, GdkEvent*, WebKitHitTestResult*, gpointer data)
    {
        // TRUE suppresses the menu; keep it only for developers (it contains "Inspect Element").
        return static_cast<LinuxWebView*>(data)->m_options.developerTools ? FALSE : TRUE;
    }

    static gboolean onPermissionRequest(WebKitWebView*, WebKitPermissionRequest* request, gpointer)
    {
        webkit_permission_request_deny(request);  // camera, geolocation, notifications, ...: never
        return TRUE;
    }

    static void onWebProcessTerminated(WebKitWebView* view, WebKitWebProcessTerminationReason reason, gpointer)
    {
        qCritical() << "WebView: web process terminated, reason" << static_cast<int>(reason) << "- reloading";
        webkit_web_view_reload(view);
    }

    static gboolean onDeleteEvent(GtkWidget*, GdkEvent*, gpointer data)
    {
        auto* self = static_cast<LinuxWebView*>(data);
        if (self->m_closeHandler && !self->m_closeHandler()) {
            return TRUE;  // keep open
        }
        gtk_widget_hide(self->m_window);
        return TRUE;  // never let GTK destroy the widget; the owner destroys it via the destructor
    }

    Options m_options;
    WebKitWebContext* m_context = nullptr;
    WebKitUserContentManager* m_contentManager = nullptr;
    WebKitWebView* m_view = nullptr;
    GtkWidget* m_window = nullptr;
};

}  // namespace

std::unique_ptr<WebView> createLinuxWebView(const Options& options, QString* error)
{
    if (!ensureGtk(error)) {
        return nullptr;
    }
    auto view = std::make_unique<LinuxWebView>(options);
    if (!view->init(error)) {
        return nullptr;
    }
    return view;
}

}  // namespace webview
