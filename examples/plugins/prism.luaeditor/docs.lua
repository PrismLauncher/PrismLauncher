-- Documentation shown by the editor and used for completion. Every name of the prism API must be here: a test checks
-- it against the real prism table. Entries with method = true document methods (folder:read, widget:text), which
-- are shown in the documentation panel but not offered as completions.
local docs = {}

local function add(name, signature, doc, extra)
    local entry = { name = name, signature = signature, doc = doc }
    for k, v in pairs(extra or {}) do
        entry[k] = v
    end
    docs[#docs + 1] = entry
end

local function api(name, signature, doc, permission)
    add(name, signature, doc, { permission = permission, group = "prism" })
end

local function method(name, signature, doc, permission)
    add(name, signature, doc, { permission = permission, group = "prism", method = true })
end

local function lua(name, signature, doc)
    add(name, signature, doc, { group = "lua" })
end

---------------------------------------------------------------------------------------------------- prism

api("prism", "prism", "The table with everything plugins can use from the launcher.")
api("prism.log", "prism.log", "Writes to the launcher log, tagged with the plugin's id.")
api("prism.log.info", "prism.log.info(...)", "Logs the arguments, joined with spaces, as information. print does the same.")
api("prism.log.warn", "prism.log.warn(...)", "Logs the arguments, joined with spaces, as a warning.")
api("prism.log.error", "prism.log.error(...)", "Logs the arguments, joined with spaces, as an error.")
api("prism.inspect", "prism.inspect(value) -> string", "A readable text version of any value, handy for logging tables.")
api("prism.settings", "prism.settings(defaults) -> settings",
    "The plugin's settings, saved in settings.ini. Each setting needs a default, which also sets its type; assigning nil restores it. Can only be called once.")
api("prism.on", "prism.on(event, [options], handler)",
    "Calls handler(data) when the event happens. options.instance limits it to one instance. Events: launcher.ready, instance.created, instance.removed, instance.status, launch.step, launch.finished, game.log.")
api("prism.async", "prism.async(fn, ...)",
    "Runs fn as a task that can wait (for prism.http) without blocking the launcher. Errors inside the task are logged.")

api("prism.instances", "prism.instances", "The launcher's instances, read only.", "instances.read")
api("prism.instances.list", "prism.instances.list() -> { instance }",
    "Every instance: { id, name, group, running, lastLaunch, playTime }, times in seconds.", "instances.read")
api("prism.instances.get", "prism.instances.get(id) -> instance | nil", "One instance, or nil if there is no such instance.", "instances.read")

api("prism.launch", "prism.launch", "Changes how instances are launched.", "launch.hooks")
api("prism.launch.addStep", "prism.launch.addStep{ before = phase | after = phase, run = function(ctx), name = text }",
    "Runs a step during every launch, before or after a phase (header, java, update, launch...). ctx.instance is the instance, ctx.log(text) writes to its log, ctx.abort(reason) stops the launch. Up to 16 steps.",
    "launch.hooks")

api("prism.fs", "prism.fs", "Files, through folders that only accept relative paths and never reach outside their root.")
api("prism.fs.data", "prism.fs.data() -> folder", "The plugin's own data folder (<plugin>/data).")
api("prism.fs.instance", "prism.fs.instance(id) -> folder", "The game folder of an instance (saves, mods, config).", "fs.instance")
api("prism.fs.plugins", "prism.fs.plugins() -> folder", "The folder with every plugin, including their code.", "fs.plugins")
api("prism.fs.copy", "prism.fs.copy(fromFolder, fromPath, toFolder, toPath)", "Copies a file or folder, also between folders. The destination must not exist.")
api("prism.fs.move", "prism.fs.move(fromFolder, fromPath, toFolder, toPath)", "Moves a file or folder, also between folders. The destination must not exist.")
method("folder:read", "folder:read(path) -> text", "The contents of a file, up to 16 MiB.")
method("folder:write", "folder:write(path, text)", "Replaces a file (atomically), creating its folders.")
method("folder:append", "folder:append(path, text)", "Adds text at the end of a file.")
method("folder:list", "folder:list([path]) -> { { name, dir, size } }", "The entries of a folder, folders first.")
method("folder:exists", "folder:exists([path]) -> boolean", "Whether the file or folder exists.")
method("folder:isDir", "folder:isDir([path]) -> boolean", "Whether the path is a folder.")
method("folder:mkdir", "folder:mkdir(path)", "Creates a folder and its parents.")
method("folder:remove", "folder:remove(path, [{ recursive = true }])", "Removes a file or an empty folder; with recursive, a folder and everything in it.")

api("prism.http", "prism.http", "Web requests, only inside tasks started with prism.async.", "net")
api("prism.http.get", "prism.http.get(url, [{ headers, timeout }]) -> response",
    "Waits for a GET request. response = { status, ok, body, headers, error }; ok is true for 2xx, error is set when nothing arrived.", "net")
api("prism.http.post", "prism.http.post(url, body, [{ headers, timeout }]) -> response", "Waits for a POST request, like prism.http.get.", "net")

api("prism.ui", "prism.ui", "Actions, notifications and windows.", "ui")
api("prism.ui.addAction", "prism.ui.addAction{ text, scope = \"launcher\" | \"instance\", run = function(ctx) }",
    "Adds an action to the Plugins menu (launcher) or to the selected instance's Plugins button (instance, with ctx.instance).", "ui")
api("prism.ui.notify", "prism.ui.notify(text, [level])", "Shows a notification; level is \"info\", \"warning\" or \"error\". At most 5 per minute.", "ui")
api("prism.ui.window", "prism.ui.window{ title, width, height, onClose } -> window",
    "Creates a window. Add widgets with window:add(kind, options) and show it with window:show().", "ui")
method("widget:add", "container:add(kind, [options]) -> widget",
    "Adds a widget to a window, row, column or split. Kinds: label, button, input, text, code, list, row, column, split (resizable panes; vertical = true stacks them). Options: text, placeholder, readOnly, stretch, items, vertical, onClick, onChange, onSelect, onFocus.")
method("widget:text", "widget:text() -> text", "The text of a label, button, input, text or code widget, or the selected item of a list.")
method("widget:setText", "widget:setText(text)", "Changes the text; for a window, its title.")
method("widget:setEnabled", "widget:setEnabled(on)", "Enables or disables the widget.")
method("widget:setVisible", "widget:setVisible(on)", "Shows or hides the widget; in a split, the other panes take its space.")
method("widget:size", "widget:size() -> width, height", "The current size of a widget or window.")
method("widget:sizes", "split:sizes() -> { sizes }", "The size of each pane of a split.")
method("widget:setSizes", "split:setSizes({ sizes })", "Resizes the panes of a split; proportions are kept if the space differs.")
method("widget:setItems", "list:setItems(items)", "Replaces the items of a list.")
method("widget:selected", "list:selected() -> index | nil", "The selected item of a list.")
method("widget:select", "list:select(index)", "Selects an item of a list.")
method("widget:setProblems", "code:setProblems({ { line, message } })", "Marks lines of a code editor; hovering them shows the message.")
method("widget:setCompletions", "code:setCompletions({ { name, signature, doc } })", "What a code editor offers while typing or with Ctrl+Space.")
method("widget:show", "window:show()", "Shows a window.")
method("widget:close", "window:close()", "Closes a window.")

api("prism.json", "prism.json", "JSON text to Lua values and back.")
api("prism.json.encode", "prism.json.encode(value, [{ pretty = true }]) -> text", "Encodes tables, strings, numbers, booleans and prism.json.null.")
api("prism.json.decode", "prism.json.decode(text) -> value", "Decodes JSON; null becomes nil. Raises an error for invalid JSON.")
api("prism.json.array", "prism.json.array([t]) -> t", "Marks a table to be encoded as an array even when it is empty.")
api("prism.json.null", "prism.json.null", "Encodes as JSON null, when a key must be kept.")

api("prism.path", "prism.path", "Helpers for paths with \"/\".")
api("prism.path.join", "prism.path.join(...) -> path", "Joins the parts with \"/\", without doubling separators.")
api("prism.path.basename", "prism.path.basename(path) -> name", "The last part: basename(\"saves/World/level.dat\") is \"level.dat\".")
api("prism.path.dirname", "prism.path.dirname(path) -> path", "Everything but the last part.")
api("prism.path.extension", "prism.path.extension(path) -> text", "The extension with its dot, e.g. \".jar\", or \"\".")
api("prism.path.stem", "prism.path.stem(path) -> name", "The last part without its extension.")

api("prism.text", "prism.text", "Helpers for text.")
api("prism.text.trim", "prism.text.trim(s) -> text", "Without spaces at both ends.")
api("prism.text.split", "prism.text.split(s, separator) -> { parts }", "Splits on a plain separator (not a pattern), keeping empty parts.")
api("prism.text.startsWith", "prism.text.startsWith(s, prefix) -> boolean", "Whether s starts with prefix.")
api("prism.text.endsWith", "prism.text.endsWith(s, suffix) -> boolean", "Whether s ends with suffix.")
api("prism.text.lines", "prism.text.lines(s) -> { lines }", "The lines of a text, accepting \\n and \\r\\n.")

---------------------------------------------------------------------------------------------------- Lua

lua("assert", "assert(v, [message], ...) -> v, ...", "Raises an error (message, or \"assertion failed!\") when v is false or nil; otherwise returns its arguments.")
lua("error", "error(message, [level])", "Raises an error. level 1 (default) points at the function calling error, 2 at its caller, 0 adds no position.")
lua("pcall", "pcall(f, ...) -> ok, ...", "Calls f in protected mode: returns true and its results, or false and the error.")
lua("xpcall", "xpcall(f, handler, ...) -> ok, ...", "Like pcall, calling handler(error) to build the error value.")
lua("type", "type(v) -> name", "\"nil\", \"number\", \"string\", \"boolean\", \"table\", \"function\", \"thread\" or \"userdata\".")
lua("tostring", "tostring(v) -> text", "Converts any value to text, using __tostring when there is one.")
lua("tonumber", "tonumber(v, [base]) -> number | nil", "Converts text to a number, or nil if it is not one.")
lua("ipairs", "ipairs(t) -> iterator", "Iterates t[1], t[2]... until the first nil: for i, v in ipairs(t) do ... end.")
lua("pairs", "pairs(t) -> iterator", "Iterates every key and value of a table, in no particular order.")
lua("next", "next(t, [key]) -> key, value", "The key after key in a table; next(t) == nil when it is empty.")
lua("select", "select(n, ...) -> ...", "The arguments from the n-th on; select(\"#\", ...) is how many there are.")
lua("print", "print(...)", "Writes the arguments to the launcher log, like prism.log.info.")
lua("rawequal", "rawequal(a, b) -> boolean", "Compares without metamethods.")
lua("rawget", "rawget(t, key) -> value", "Reads a table without metamethods.")
lua("rawset", "rawset(t, key, value) -> t", "Writes a table without metamethods.")
lua("rawlen", "rawlen(v) -> length", "The length of a table or string, without metamethods.")
lua("setmetatable", "setmetatable(t, metatable) -> t", "Sets the metatable of a table. __gc is not allowed in plugins.")
lua("getmetatable", "getmetatable(v) -> metatable", "The metatable of a value (or its __metatable field).")
lua("load", "load(chunk, [name], [mode], [env]) -> function | nil, error", "Compiles Lua text without running it; plugins cannot load bytecode.")
lua("require", "require(name) -> module", "Loads a module of the plugin: require(\"a.b\") runs a/b.lua (or a/b/init.lua) once and returns its result.")
lua("collectgarbage", "collectgarbage([option]) -> ...", "Controls the garbage collector, e.g. collectgarbage(\"count\") is the memory in use in KiB.")

lua("string.format", "string.format(format, ...) -> text", "Formats like C printf: %d, %s, %q, %5.2f...")
lua("string.find", "string.find(s, pattern, [init], [plain]) -> start, end, ...", "Where a pattern (or plain text) appears, and its captures.")
lua("string.match", "string.match(s, pattern, [init]) -> ...", "The captures (or the whole match) of the first match of a pattern.")
lua("string.gmatch", "string.gmatch(s, pattern) -> iterator", "Iterates the matches of a pattern: for word in s:gmatch(\"%a+\") do ... end.")
lua("string.gsub", "string.gsub(s, pattern, replacement, [n]) -> text, count", "Replaces matches of a pattern with text, a table or a function's result.")
lua("string.sub", "string.sub(s, i, [j]) -> text", "The part from i to j (negative counts from the end).")
lua("string.len", "string.len(s) -> length", "The length in bytes, the same as #s.")
lua("string.lower", "string.lower(s) -> text", "In lower case.")
lua("string.upper", "string.upper(s) -> text", "In upper case.")
lua("string.rep", "string.rep(s, n, [separator]) -> text", "s repeated n times.")
lua("string.reverse", "string.reverse(s) -> text", "The bytes in reverse order.")
lua("string.byte", "string.byte(s, [i], [j]) -> codes", "The byte values of characters.")
lua("string.char", "string.char(...) -> text", "Text made of the given byte values.")
lua("string.pack", "string.pack(format, ...) -> bytes", "Packs values into binary data.")
lua("string.unpack", "string.unpack(format, bytes, [pos]) -> ...", "Reads values from binary data.")
lua("string.packsize", "string.packsize(format) -> size", "The size of a binary format.")
lua("string.dump", "string.dump(f) -> bytes", "The bytecode of a function (plugins cannot load it back).")

lua("table.insert", "table.insert(t, [pos], value)", "Inserts a value at the end, or at pos moving the rest up.")
lua("table.remove", "table.remove(t, [pos]) -> value", "Removes the last value, or the one at pos, and returns it.")
lua("table.concat", "table.concat(t, [separator], [i], [j]) -> text", "Joins strings and numbers of a list.")
lua("table.sort", "table.sort(t, [less])", "Sorts a list in place, optionally with less(a, b).")
lua("table.unpack", "table.unpack(t, [i], [j]) -> ...", "The values of a list as separate values.")
lua("table.pack", "table.pack(...) -> t", "A list with the arguments and their count in t.n.")
lua("table.move", "table.move(a, from, to, start, [b]) -> b", "Copies a range of a list into another position or list.")

lua("math.floor", "math.floor(x) -> integer", "The largest integer not above x.")
lua("math.ceil", "math.ceil(x) -> integer", "The smallest integer not below x.")
lua("math.abs", "math.abs(x) -> number", "The absolute value.")
lua("math.max", "math.max(x, ...) -> number", "The largest argument.")
lua("math.min", "math.min(x, ...) -> number", "The smallest argument.")
lua("math.fmod", "math.fmod(x, y) -> number", "The remainder of x / y, rounded towards zero.")
lua("math.modf", "math.modf(x) -> integer part, fraction", "Splits a number in its integer and fractional parts.")
lua("math.sqrt", "math.sqrt(x) -> number", "The square root.")
lua("math.exp", "math.exp(x) -> number", "e raised to x.")
lua("math.log", "math.log(x, [base]) -> number", "The logarithm (natural by default).")
lua("math.sin", "math.sin(x) -> number", "The sine, in radians.")
lua("math.cos", "math.cos(x) -> number", "The cosine, in radians.")
lua("math.tan", "math.tan(x) -> number", "The tangent, in radians.")
lua("math.asin", "math.asin(x) -> number", "The arc sine, in radians.")
lua("math.acos", "math.acos(x) -> number", "The arc cosine, in radians.")
lua("math.atan", "math.atan(y, [x]) -> number", "The arc tangent of y/x, in radians.")
lua("math.random", "math.random([m], [n]) -> number", "A random float in [0, 1), or integer in [1, m] or [m, n].")
lua("math.randomseed", "math.randomseed([x], [y])", "Seeds the random generator.")
lua("math.tointeger", "math.tointeger(x) -> integer | nil", "x as an integer, if it has an exact one.")
lua("math.type", "math.type(x) -> \"integer\" | \"float\" | nil", "Whether a number is an integer or a float.")
lua("math.ult", "math.ult(a, b) -> boolean", "a < b comparing as unsigned integers.")
lua("math.pi", "math.pi", "The value of pi.")
lua("math.huge", "math.huge", "A value larger than any number (infinity).")
lua("math.maxinteger", "math.maxinteger", "The largest integer.")
lua("math.mininteger", "math.mininteger", "The smallest integer.")

lua("utf8.char", "utf8.char(...) -> text", "Text with the given code points.")
lua("utf8.codepoint", "utf8.codepoint(s, [i], [j]) -> codes", "The code points of the characters.")
lua("utf8.codes", "utf8.codes(s) -> iterator", "Iterates positions and code points: for pos, code in utf8.codes(s) do ... end.")
lua("utf8.len", "utf8.len(s, [i], [j]) -> length | nil", "The number of characters (not bytes), or nil for invalid UTF-8.")
lua("utf8.offset", "utf8.offset(s, n, [i]) -> position", "The byte position of the n-th character.")
lua("utf8.charpattern", "utf8.charpattern", "A pattern matching one UTF-8 character.")

lua("coroutine.create", "coroutine.create(f) -> thread", "A coroutine running f.")
lua("coroutine.resume", "coroutine.resume(co, ...) -> ok, ...", "Starts or continues a coroutine.")
lua("coroutine.yield", "coroutine.yield(...) -> ...", "Pauses the running coroutine.")
lua("coroutine.wrap", "coroutine.wrap(f) -> function", "A function that resumes a coroutine running f each time it is called.")
lua("coroutine.status", "coroutine.status(co) -> status", "\"running\", \"suspended\", \"normal\" or \"dead\".")
lua("coroutine.running", "coroutine.running() -> thread, ismain", "The running coroutine.")
lua("coroutine.isyieldable", "coroutine.isyieldable() -> boolean", "Whether the running code can yield.")
lua("coroutine.close", "coroutine.close(co) -> ok, error", "Closes a suspended or dead coroutine.")

return docs
