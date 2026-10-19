// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here only hold plain pointers across
// Lua calls that may raise errors: text going to Lua passes through a static buffer (plugins only run on the main
// thread), and C++ objects are only created in statements that make no such call. Callbacks from Qt run in protected
// mode through LuaState::call.

#include "UiApi.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include <cstring>
#include <memory>
#include <variant>

#include <lua.hpp>

#include "plugins/Logging.h"
#include "plugins/lua/LuaState.h"
#include "ui/widgets/LuaCodeEditor.h"

namespace {

constexpr const char* s_widgetMetatable = "prism.Widget";
constexpr const char* s_windowsMetatable = "prism.Windows";
// Registry key of the userdata that closes the open windows when the state is closed
constexpr const char* s_windowsKey = "prism.windows";
// Property of each widget naming its kind
constexpr const char* s_kindProperty = "prismKind";
constexpr int s_maxWindows = 8;

QByteArray s_text;
QList<LuaCodeEditor::Problem> s_problems;

struct WidgetHandle {
    QPointer<QWidget>* widget;
};

struct OpenWindows {
    QList<QPointer<QDialog>>* windows;
};

using CallbackArg = std::variant<QByteArray, qint64>;

struct CallbackArgs {
    int ref;
    const QList<CallbackArg>* args;
};

// Calls the function referenced by ref. Argument 1 is a light userdata pointing to the CallbackArgs.
int invokeCallback(lua_State* L)
{
    const auto* callback = static_cast<const CallbackArgs*>(lua_touserdata(L, 1));
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback->ref);
    for (const auto& arg : *callback->args) {
        if (const auto* text = std::get_if<QByteArray>(&arg)) {
            lua_pushlstring(L, text->constData(), text->size());
        } else {
            lua_pushinteger(L, std::get<qint64>(arg));
        }
    }
    lua_call(L, static_cast<int>(callback->args->size()), 0);
    return 0;
}

// Calls back into Lua from a Qt signal, unless the state is gone
void callBack(LuaState* state, const std::weak_ptr<void>& lifetime, int ref, const QList<CallbackArg>& args = {})
{
    if (lifetime.expired() || ref == LUA_NOREF) {
        return;
    }
    CallbackArgs callback{ ref, &args };
    lua_pushcfunction(state->get(), invokeCallback);
    lua_pushlightuserdata(state->get(), &callback);
    if (auto error = state->call(1)) {
        qCWarning(pluginLogC).noquote() << "Plugin window callback failed:" << *error;
    }
}

// A reference to the function in field name of the table at index, or LUA_NOREF if there is none
int callbackRef(lua_State* L, int index, const char* name)
{
    if (!lua_istable(L, index)) {
        return LUA_NOREF;
    }
    const int type = lua_getfield(L, index, name);
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        return LUA_NOREF;
    }
    if (type != LUA_TFUNCTION) {
        luaL_error(L, "'%s' must be a function", name);
    }
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// The string in field name of the table at index, or fallback
const char* optField(lua_State* L, int index, const char* name, const char* fallback)
{
    if (!lua_istable(L, index)) {
        return fallback;
    }
    const int type = lua_getfield(L, index, name);
    if (type != LUA_TNIL && type != LUA_TSTRING) {
        luaL_error(L, "'%s' must be a string", name);
    }
    // Not converted, so the table still references the string and the pointer stays valid
    const char* value = type == LUA_TNIL ? fallback : lua_tostring(L, -1);
    lua_pop(L, 1);
    return value;
}

bool optBoolField(lua_State* L, int index, const char* name)
{
    if (!lua_istable(L, index)) {
        return false;
    }
    lua_getfield(L, index, name);
    const bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

lua_Integer optIntField(lua_State* L, int index, const char* name, lua_Integer fallback)
{
    if (!lua_istable(L, index)) {
        return fallback;
    }
    lua_getfield(L, index, name);
    const lua_Integer value = lua_isnil(L, -1) ? fallback : luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

void pushHandle(lua_State* L, QWidget* widget)
{
    auto* handle = static_cast<WidgetHandle*>(lua_newuserdatauv(L, sizeof(WidgetHandle), 0));
    handle->widget = nullptr;
    luaL_setmetatable(L, s_widgetMetatable);
    handle->widget = new QPointer<QWidget>(widget);
}

// The widget of the handle at index; raises an error if it was closed
QWidget* checkWidget(lua_State* L, int index)
{
    auto* handle = static_cast<WidgetHandle*>(luaL_checkudata(L, index, s_widgetMetatable));
    QWidget* widget = handle->widget ? handle->widget->data() : nullptr;
    if (!widget) {
        luaL_error(L, "the window was closed");
    }
    return widget;
}

QByteArray kindOf(QWidget* widget)
{
    return widget->property(s_kindProperty).toByteArray();
}

// Raises an error unless the widget is one of the kinds (a space separated list)
template <typename T>
T* checkKind(lua_State* L, int index, const char* method)
{
    QWidget* widget = checkWidget(L, index);
    if (auto* typed = qobject_cast<T*>(widget)) {
        return typed;
    }
    s_text = kindOf(widget);
    luaL_error(L, "a %s has no method '%s'", s_text.constData(), method);
    return nullptr;
}

void setItems(lua_State* L, QListWidget* list, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    list->clear();
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, index));
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, index, i);
        size_t length = 0;
        const char* text = luaL_tolstring(L, -1, &length);
        list->addItem(QString::fromUtf8(text, static_cast<qsizetype>(length)));
        lua_pop(L, 2);
    }
}

void setProblems(lua_State* L, LuaCodeEditor* editor, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    // Validated first, so the C++ list below is built without Lua calls that raise errors
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, index));
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, index, i);
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_getfield(L, -1, "line");
        lua_getfield(L, -2, "message");
        if (!lua_isinteger(L, -2) || lua_type(L, -1) != LUA_TSTRING) {
            luaL_error(L, "problems must be tables with a line number and a message");
        }
        lua_pop(L, 3);
    }
    s_problems.clear();
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, index, i);
        lua_getfield(L, -1, "line");
        lua_getfield(L, -2, "message");
        s_problems.append({ static_cast<int>(lua_tointeger(L, -2)), QString::fromUtf8(lua_tostring(L, -1)) });
        lua_pop(L, 3);
    }
    editor->setProblems(s_problems);
    s_problems.clear();
}

struct WidgetOptions {
    const char* kind;
    const char* text;
    const char* placeholder;
    bool readOnly;
    int onClick;
    int onChange;
    int onSelect;
};

bool isKnownKind(const char* kind)
{
    for (const char* known : { "label", "button", "input", "text", "code", "list", "row", "column" }) {
        if (std::strcmp(kind, known) == 0) {
            return true;
        }
    }
    return false;
}

// Creates the widget; makes no Lua call that raises errors
QWidget* createWidget(LuaState* state, const WidgetOptions& options)
{
    const auto lifetime = state->lifetime();
    const char* kind = options.kind;
    QWidget* widget = nullptr;
    if (std::strcmp(kind, "label") == 0) {
        auto* label = new QLabel(QString::fromUtf8(options.text));
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        widget = label;
    } else if (std::strcmp(kind, "button") == 0) {
        auto* button = new QPushButton(QString::fromUtf8(options.text));
        QObject::connect(button, &QPushButton::clicked, button,
                         [state, lifetime, ref = options.onClick] { callBack(state, lifetime, ref); });
        widget = button;
    } else if (std::strcmp(kind, "input") == 0) {
        auto* input = new QLineEdit(QString::fromUtf8(options.text));
        input->setPlaceholderText(QString::fromUtf8(options.placeholder));
        input->setReadOnly(options.readOnly);
        QObject::connect(input, &QLineEdit::textEdited, input,
                         [state, lifetime, ref = options.onChange] { callBack(state, lifetime, ref); });
        QObject::connect(input, &QLineEdit::returnPressed, input,
                         [state, lifetime, ref = options.onClick] { callBack(state, lifetime, ref); });
        widget = input;
    } else if (std::strcmp(kind, "text") == 0 || std::strcmp(kind, "code") == 0) {
        QPlainTextEdit* edit = std::strcmp(kind, "code") == 0 ? new LuaCodeEditor : new QPlainTextEdit;
        edit->setPlainText(QString::fromUtf8(options.text));
        edit->setPlaceholderText(QString::fromUtf8(options.placeholder));
        edit->setReadOnly(options.readOnly);
        QObject::connect(edit, &QPlainTextEdit::textChanged, edit,
                         [state, lifetime, ref = options.onChange] { callBack(state, lifetime, ref); });
        widget = edit;
    } else if (std::strcmp(kind, "list") == 0) {
        auto* list = new QListWidget;
        QObject::connect(list, &QListWidget::currentRowChanged, list, [state, lifetime, ref = options.onSelect, list](int row) {
            if (row >= 0) {
                callBack(state, lifetime, ref, { qint64(row + 1), list->item(row)->text().toUtf8() });
            }
        });
        widget = list;
    } else {
        widget = new QWidget;
        QBoxLayout* layout = std::strcmp(kind, "row") == 0 ? static_cast<QBoxLayout*>(new QHBoxLayout(widget)) : new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    widget->setProperty(s_kindProperty, QByteArray(kind));
    return widget;
}

// container:add(kind, opts): creates a widget in the container and returns its handle. opts: text, placeholder,
// readOnly, stretch, items (list), onClick, onChange, onSelect.
int addWidget(lua_State* L)
{
    QWidget* container = checkWidget(L, 1);
    if (!container->layout()) {
        s_text = kindOf(container);
        return luaL_error(L, "a %s cannot contain widgets", s_text.constData());
    }
    WidgetOptions options{};
    options.kind = luaL_checkstring(L, 2);
    if (!isKnownKind(options.kind)) {
        return luaL_error(L, "unknown widget kind '%s'", options.kind);
    }
    if (!lua_isnoneornil(L, 3)) {
        luaL_checktype(L, 3, LUA_TTABLE);
    }
    options.text = optField(L, 3, "text", "");
    options.placeholder = optField(L, 3, "placeholder", "");
    options.readOnly = optBoolField(L, 3, "readOnly");
    const lua_Integer stretch = optIntField(L, 3, "stretch", 0);
    options.onClick = callbackRef(L, 3, "onClick");
    options.onChange = callbackRef(L, 3, "onChange");
    options.onSelect = callbackRef(L, 3, "onSelect");

    QWidget* widget = createWidget(LuaState::fromLua(L), options);
    static_cast<QBoxLayout*>(container->layout())->addWidget(widget, static_cast<int>(stretch));

    pushHandle(L, widget);
    if (auto* list = qobject_cast<QListWidget*>(widget); list && lua_istable(L, 3)) {
        lua_getfield(L, 3, "items");
        if (!lua_isnil(L, -1)) {
            setItems(L, list, lua_gettop(L));
        }
        lua_pop(L, 1);
    }
    return 1;
}

int widgetText(lua_State* L)
{
    QWidget* widget = checkWidget(L, 1);
    if (auto* edit = qobject_cast<QPlainTextEdit*>(widget)) {
        s_text = edit->toPlainText().toUtf8();
    } else if (auto* input = qobject_cast<QLineEdit*>(widget)) {
        s_text = input->text().toUtf8();
    } else if (auto* label = qobject_cast<QLabel*>(widget)) {
        s_text = label->text().toUtf8();
    } else if (auto* button = qobject_cast<QPushButton*>(widget)) {
        s_text = button->text().toUtf8();
    } else if (auto* list = qobject_cast<QListWidget*>(widget)) {
        s_text = list->currentItem() ? list->currentItem()->text().toUtf8() : QByteArray();
    } else {
        s_text = kindOf(widget);
        return luaL_error(L, "a %s has no text", s_text.constData());
    }
    lua_pushlstring(L, s_text.constData(), s_text.size());
    s_text.clear();
    return 1;
}

int widgetSetText(lua_State* L)
{
    QWidget* widget = checkWidget(L, 1);
    size_t length = 0;
    const char* text = luaL_checklstring(L, 2, &length);
    const QString value = QString::fromUtf8(text, static_cast<qsizetype>(length));
    // No Lua calls from here on (the signals below may call back, but in protected mode)
    if (auto* edit = qobject_cast<QPlainTextEdit*>(widget)) {
        edit->setPlainText(value);
    } else if (auto* input = qobject_cast<QLineEdit*>(widget)) {
        input->setText(value);
    } else if (auto* label = qobject_cast<QLabel*>(widget)) {
        label->setText(value);
    } else if (auto* button = qobject_cast<QPushButton*>(widget)) {
        button->setText(value);
    } else if (auto* dialog = qobject_cast<QDialog*>(widget)) {
        dialog->setWindowTitle(value);
    } else {
        return 0;
    }
    return 0;
}

int widgetSetEnabled(lua_State* L)
{
    checkWidget(L, 1)->setEnabled(lua_toboolean(L, 2));
    return 0;
}

int widgetSetVisible(lua_State* L)
{
    QWidget* widget = checkWidget(L, 1);
    if (qobject_cast<QDialog*>(widget)) {
        return luaL_error(L, "use show and close for windows");
    }
    widget->setVisible(lua_toboolean(L, 2));
    return 0;
}

int widgetSetItems(lua_State* L)
{
    setItems(L, checkKind<QListWidget>(L, 1, "setItems"), 2);
    return 0;
}

int widgetSelected(lua_State* L)
{
    auto* list = checkKind<QListWidget>(L, 1, "selected");
    const int row = list->currentRow();
    if (row < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, row + 1);
    return 1;
}

int widgetSelect(lua_State* L)
{
    auto* list = checkKind<QListWidget>(L, 1, "select");
    const lua_Integer row = luaL_checkinteger(L, 2);
    list->setCurrentRow(static_cast<int>(row - 1));
    return 0;
}

int widgetSetProblems(lua_State* L)
{
    setProblems(L, checkKind<LuaCodeEditor>(L, 1, "setProblems"), 2);
    return 0;
}

int widgetShow(lua_State* L)
{
    auto* dialog = checkKind<QDialog>(L, 1, "show");
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return 0;
}

int widgetClose(lua_State* L)
{
    checkKind<QDialog>(L, 1, "close")->close();
    return 0;
}

int widgetGc(lua_State* L)
{
    auto* handle = static_cast<WidgetHandle*>(lua_touserdata(L, 1));
    delete handle->widget;
    handle->widget = nullptr;
    return 0;
}

int windowsGc(lua_State* L)
{
    auto* open = static_cast<OpenWindows*>(lua_touserdata(L, 1));
    if (open->windows) {
        for (const auto& window : *open->windows) {
            delete window.data();
        }
        delete open->windows;
        open->windows = nullptr;
    }
    return 0;
}

// The list of open windows of the state, created on first use
QList<QPointer<QDialog>>* openWindows(lua_State* L)
{
    if (lua_getfield(L, LUA_REGISTRYINDEX, s_windowsKey) == LUA_TUSERDATA) {
        auto* open = static_cast<OpenWindows*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return open->windows;
    }
    lua_pop(L, 1);
    auto* open = static_cast<OpenWindows*>(lua_newuserdatauv(L, sizeof(OpenWindows), 0));
    open->windows = nullptr;
    luaL_setmetatable(L, s_windowsMetatable);
    lua_setfield(L, LUA_REGISTRYINDEX, s_windowsKey);
    open->windows = new QList<QPointer<QDialog>>;
    return open->windows;
}

// prism.ui.window{ title, width, height, onClose }: upvalue 1 tells whether the plugin has the ui permission
int window(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.ui.window requires the 'ui' permission");
    }
    if (!lua_isnoneornil(L, 1)) {
        luaL_checktype(L, 1, LUA_TTABLE);
    }
    const char* title = optField(L, 1, "title", "");
    const lua_Integer width = optIntField(L, 1, "width", 500);
    const lua_Integer height = optIntField(L, 1, "height", 400);
    const int onClose = callbackRef(L, 1, "onClose");
    QList<QPointer<QDialog>>* windows = openWindows(L);
    windows->removeIf([](const QPointer<QDialog>& window) { return window.isNull(); });
    if (windows->size() >= s_maxWindows) {
        return luaL_error(L, "too many open windows, the maximum is %d", s_maxWindows);
    }

    LuaState* state = LuaState::fromLua(L);
    auto* dialog = new QDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(QString::fromUtf8(title));
    dialog->resize(static_cast<int>(width), static_cast<int>(height));
    dialog->setProperty(s_kindProperty, QByteArray("window"));
    new QVBoxLayout(dialog);
    QObject::connect(dialog, &QDialog::finished, dialog,
                     [state, lifetime = state->lifetime(), onClose] { callBack(state, lifetime, onClose); });
    windows->append(dialog);

    pushHandle(L, dialog);
    return 1;
}

}  // namespace

namespace PrismApi {

void addWindowFunction(lua_State* L, bool allowed)
{
    if (luaL_newmetatable(L, s_widgetMetatable)) {
        const luaL_Reg methods[] = {
            { "add", addWidget },
            { "text", widgetText },
            { "setText", widgetSetText },
            { "setEnabled", widgetSetEnabled },
            { "setVisible", widgetSetVisible },
            { "setItems", widgetSetItems },
            { "selected", widgetSelected },
            { "select", widgetSelect },
            { "setProblems", widgetSetProblems },
            { "show", widgetShow },
            { "close", widgetClose },
            { nullptr, nullptr },
        };
        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, widgetGc);
        lua_setfield(L, -2, "__gc");
        lua_pushboolean(L, false);
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);
    if (luaL_newmetatable(L, s_windowsMetatable)) {
        lua_pushcfunction(L, windowsGc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);

    lua_pushboolean(L, allowed);
    lua_pushcclosure(L, window, 1);
    lua_setfield(L, -2, "window");
}

}  // namespace PrismApi
