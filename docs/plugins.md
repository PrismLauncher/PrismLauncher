# Writing plugins

Plugins are Lua 5.5 scripts that extend the launcher: they react to events, add steps to the launch of an instance,
add buttons, menus and windows, read and write files, and talk to the network. Everything goes through the `prism`
table, and anything beyond basic scripting needs a permission the user grants when enabling the plugin.

## Contents

- [Quick start](#quick-start)
- [The manifest](#the-manifest)
- [Permissions](#permissions)
- [Sandbox and limits](#sandbox-and-limits)
- [API reference](#api-reference)
- [Events](#events)
- [Launch phases](#launch-phases)
- [Examples](#examples)
- [Tips and pitfalls](#tips-and-pitfalls)

## Quick start

1. Open the plugins folder: Settings → Plugins → Open Folder (it is `<data folder>/plugins`). To share a plugin, zip
   its folder: Settings → Plugins → Install from Zip installs it, and replaces an older version keeping its
   `settings.ini` and data folder.
2. Create a folder named after your plugin's id, for example `com.example.hello`.
3. Add `plugin.toml`:

   ```toml
   id = "com.example.hello"
   name = "Hello"
   version = "1.0.0"
   api = 1
   permissions = ["ui"]
   ```

4. Add `main.lua`:

   ```lua
   prism.log.info("Hello from my plugin")

   prism.on("launcher.ready", function()
       prism.ui.notify("The launcher is ready!")
   end)
   ```

5. Press Reload in Settings → Plugins, tick your plugin and accept its permissions. Its messages appear in the launcher
   log (Help → View logs), tagged with its id.

New plugins always start disabled: copying a folder is never enough to run code.

The [Lua editor](../examples/plugins/prism.luaeditor) example plugin edits the code of the installed plugins and points
out syntax errors and misspelled `prism.*` names while you type. Copy its folder into the plugins folder to use it.

## The manifest

`plugin.toml` describes the plugin:

| Field | Required | Description |
| --- | --- | --- |
| `id` | yes | Up to 64 characters `a-z`, `0-9`, `_`, `-`, separated by dots. Must match the folder name. |
| `name` | yes | Shown in the launcher. |
| `version` | yes | Any non-empty text. |
| `api` | yes | The API version the plugin is written for. Currently `1`. |
| `entry` | no | The script to run, relative to the folder. Defaults to `main.lua`. |
| `permissions` | no | What the plugin needs, see below. |

Unknown fields are errors, so a typo never goes unnoticed.

## Permissions

| Permission | Grants |
| --- | --- |
| `instances.read` | `prism.instances`, and the instance and launch events. |
| `launch.hooks` | `prism.launch.addStep`: run steps during a launch and stop it. |
| `game.log` | The `game.log` event: the game's output while it runs. |
| `fs.instance` | `prism.fs.instance(id)`: the game folders of instances. |
| `fs.plugins` | `prism.fs.plugins()`: the folder with every plugin, including their code. |
| `net` | `prism.http`. |
| `ui` | `prism.ui`: actions, notifications and windows. |
| `unrestricted` | Full Lua: `io`, `os` and `package`. Only for plugins the user fully trusts. |

When a plugin is enabled, the launcher lists its permissions and only enables it if the user accepts them. If an update
asks for more, the plugin is not loaded until the user approves them again.

## Sandbox and limits

Plugins get the Lua standard libraries `string`, `table`, `math`, `utf8`, `coroutine` and the base functions, except:

- `load` only accepts text, never bytecode.
- `dofile` and `loadfile` are not available.
- `setmetatable` rejects `__gc` metamethods.
- `io`, `os`, `package` and `debug` are only available with `unrestricted` (and `debug` never is).

`require` loads modules from the plugin's own folder: `require("util.strings")` loads `util/strings.lua` or
`util/strings/init.lua`.

Each plugin has its own Lua state, limited to 32 MiB of memory. Each call into a plugin (loading it, an event, a
callback) may run 10 million instructions; past that, it fails with `instruction limit exceeded`. Plugins run on the
launcher's main thread, so long work should be split or wait with `prism.async`.

Errors in event handlers, callbacks and tasks are logged and do not unload the plugin. Errors while loading put it in
the Error state, shown in Settings → Plugins.

## API reference

### Log

- `prism.log.info(...)`, `prism.log.warn(...)`, `prism.log.error(...)`: write to the launcher log. Arguments are joined
  with spaces, like `print`, which is the same as `prism.log.info`. Messages are cut at 4096 characters.
- `prism.inspect(value)`: a readable text version of any value, handy for logging tables.

### Settings

`prism.settings(defaults)` returns the plugin's settings, stored in `settings.ini` in its folder. It can only be called
once. Every setting needs a default, which also sets its type:

```lua
local settings = prism.settings{ interval = 10, enabled = true, name = "backup" }
settings.interval = 30      -- saved
settings.interval = "30"    -- error: must be a number
settings.name = nil         -- back to "backup"
```

Up to 256 settings; text values up to 64 KiB.

### Event handlers

`prism.on(event, [options], handler)` calls `handler(data)` when the event happens. With `options.instance` set to an
instance id, it only runs for that instance. See [Events](#events) for the list.

### Instances

Needs `instances.read`.

- `prism.instances.list()`: every instance.
- `prism.instances.get(id)`: one instance, or `nil`.

Each instance is a table `{ id, name, group, running, lastLaunch, playTime }`; times are in seconds.

### Launch steps

Needs `launch.hooks`. `prism.launch.addStep{ before = phase | after = phase, run = function(ctx), name = text }` runs
`run` during every launch, before or after one of the [launch phases](#launch-phases):

- `ctx.instance` is the instance being launched (`id` and `name`).
- `ctx.log(text)` writes to the instance's log window.
- `ctx.abort(reason)` stops the launch.

If `run` fails, the launch goes on and the error is shown in the log. Up to 16 steps.

### Files

`prism.fs` works with folders. Paths are relative and use `/`; nothing outside the folder can be reached, not even
through symbolic links.

- `prism.fs.data()`: the plugin's own data folder (`<plugin>/data`). Always available.
- `prism.fs.instance(id)`: the game folder of an instance (saves, mods, config). Needs `fs.instance`.
- `prism.fs.plugins()`: the plugins folder. Needs `fs.plugins`.

Folder methods: `read(path)` (up to 16 MiB), `write(path, text)` (atomic), `append(path, text)`, `list([path])` (entries
`{ name, dir, size }`), `exists([path])`, `isDir([path])`, `mkdir(path)`, `remove(path, { recursive = true })`.
`prism.fs.copy(fromFolder, fromPath, toFolder, toPath)` and `prism.fs.move(...)` work across folders. Failures raise
errors.

### Tasks and HTTP

`prism.async(fn, ...)` runs `fn` as a task that can wait without blocking the launcher. Inside a task,
`prism.http.get(url, [options])` and `prism.http.post(url, body, [options])` (need `net`) wait for the response and
return `{ status, ok, body, headers, error }`:

- `ok` is true for 2xx statuses; `error` is set when no response arrived.
- `headers` uses lower case names.
- `options.headers` adds request headers; `options.timeout` is in seconds (default 30, at most 120).

Only `http` and `https`, responses up to 16 MiB, at most 8 requests at a time. Requests use the launcher's proxy
settings.

### User interface

Needs `ui`.

- `prism.ui.notify(text, [level])` shows a notification in the main window; `level` is `"info"`, `"warning"` or
  `"error"`. At most 5 per minute.
- `prism.ui.addAction{ text, scope, run }` adds an action. With `scope = "launcher"` it goes to the Plugins menu and
  toolbar button; with `scope = "instance"` to the Plugins button of the selected instance, and `run(ctx)` gets
  `ctx.instance`.
- `prism.ui.window{ title, width, height, onClose }` creates a window:

  ```lua
  local win = prism.ui.window{ title = "My plugin", width = 600, height = 400 }
  local name = win:add("input", { placeholder = "Your name" })
  win:add("button", { text = "Greet", onClick = function()
      prism.ui.notify("Hello, " .. name:text())
  end })
  win:show()
  ```

  Widget kinds for `container:add(kind, options)`: `label`, `button`, `input` (one line), `text` (several lines),
  `code` (Lua editor), `list`, `combo` (drop-down list), `tree` (items `{ text, id, children, expanded }`), and the
  containers `row`, `column` and `split` (panes the user resizes by dragging;
  `vertical = true` stacks them). Options: `text`, `placeholder`, `readOnly`,
  `stretch` (share of the space), `items` (lists), `onClick` (buttons, and Enter in inputs), `onChange` (inputs and
  editors), `onSelect` (lists and combos: `function(index, text)`; trees: `function(id, text)`, with an empty id for
  items without one), `onFocus` (code editors, `function(name)`: the name under the
  cursor or highlighted in the completion list).

  Methods: `text()`, `setText(text)`, `setEnabled(on)`, `setVisible(on)`, `setItems(items)`, `selected()`,
  `select(index)` (`select(id)` in trees, which also opens their folders), `size()` (width and height, also of windows), `sizes()` and `setSizes({ ... })` (panes of a split),
  `addShortcut(keys, fn)` (windows: `keys` like `"Ctrl+S"`), `setProblems({ { line, message } })` and `setCompletions({ { name, signature, doc } })` (code
  editors: offered while typing or with Ctrl+Space, with the names used in the text), and `show()` and `close()`
  (windows). Code editors copy (Ctrl+C) and cut (Ctrl+X) the whole line when nothing is selected, and paste such a
  line below the current one (Ctrl+V). A plugin
  can have up to 8 windows open; they close when it is disabled.

### Helpers

- `prism.json.encode(value, [{ pretty = true }])` and `prism.json.decode(text)`. `null` decodes to `nil`;
  `prism.json.array(t)` makes an empty table encode as `[]`, and `prism.json.null` encodes as `null`.
- `prism.path.join(...)`, `basename(path)`, `dirname(path)`, `extension(path)`, `stem(path)`.
- `prism.text.trim(s)`, `split(s, separator)`, `startsWith(s, prefix)`, `endsWith(s, suffix)`, `lines(s)`.

## Events

| Event | Data | Permission |
| --- | --- | --- |
| `launcher.ready` | none | none |
| `instance.created` | `{ id, name }` | `instances.read` |
| `instance.removed` | `{ id, name }` | `instances.read` |
| `instance.status` | `{ id, running }` | `instances.read` |
| `launch.step` | `{ id, step }`: a launch phase starts | `instances.read` |
| `launch.finished` | `{ id, success, reason }`: the game session ended | `instances.read` |
| `game.log` | `{ id, line, level, skipped }` | `game.log` |

`launcher.ready` also reaches plugins enabled after the launcher started. `game.log` only carries what the game prints,
with private information such as access tokens removed, and at most 200 lines per second; `skipped` counts the lines
dropped just before.

## Launch phases

In order: `header`, `create-folders`, `lookup-server`, `pre-load-command`, `load-meta`, `java`, `pre-launch-command`,
`update`, `jar-mods`, `scan-mods`, `check-memory`, `instance-info`, `natives`, `assets`, `launch`,
`post-exit-command`, `quit-after-stop`.

`lookup-server`, `pre-load-command`, `pre-launch-command`, `post-exit-command` and `quit-after-stop` only exist when
their setting or situation applies; steps anchored to a missing phase are skipped. `java` and `update` are made of
several steps: "before" goes before the first one, "after" after the last one. `launch` is the game itself.

## Examples

### Back up the worlds before playing

```toml
id = "com.example.worldbackup"
name = "World backup"
version = "1.0.0"
api = 1
permissions = ["launch.hooks", "fs.instance"]
```

```lua
prism.launch.addStep{ before = "launch", name = "Back up worlds", run = function(ctx)
    local game = prism.fs.instance(ctx.instance.id)
    if not game:isDir("saves") then
        return
    end
    local data = prism.fs.data()
    local target = prism.path.join(ctx.instance.id, "backup")
    if data:exists(target) then
        data:remove(target, { recursive = true })
    end
    prism.fs.copy(game, "saves", data, target)
    ctx.log("Worlds backed up")
end }
```

### Check for news on launch

```lua
-- permissions = ["net", "ui"]
prism.on("launcher.ready", function()
    prism.async(function()
        local response = prism.http.get("https://example.com/news.json", { headers = { Accept = "application/json" } })
        if not response.ok then
            prism.log.warn("Could not get the news:", response.status, response.error)
            return
        end
        local news = prism.json.decode(response.body)
        prism.ui.notify(news.title)
    end)
end)
```

### Count play sessions

```lua
-- permissions = ["instances.read"]
local settings = prism.settings{ sessions = 0 }
prism.on("launch.finished", function(e)
    if e.success then
        settings.sessions = settings.sessions + 1
        prism.log.info("Sessions played:", settings.sessions)
    end
end)
```

## Tips and pitfalls

- In Lua 5.5 the variables of a `for` loop are constant: `for name in ... do name = name:lower() end` does not compile.
  Use a new local instead.
- `print` goes to the launcher log, not a console.
- Enabled plugins restart by themselves when their `.lua` files or `plugin.toml` change (Settings → Plugins → Reload
  Plugin does it by hand). What a plugin writes itself (`settings.ini`, its data folder) does not restart it.
- Keep work per call small: it runs on the launcher's main thread, within the instruction limit.
- `prism.http` only works inside `prism.async`; errors inside a task are logged, not raised to whoever started it.
- With `unrestricted`, native modules (`.so`, `.dll`) cannot be loaded, and on Windows `io.open` fails with paths that
  are not plain ASCII; use `prism.fs` for files.
