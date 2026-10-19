-- A small editor for the Lua files of the installed plugins, which points out problems while typing, completes names
-- and shows the documentation of the prism API and of Lua.
local checks = require("checks")
local docs = require("docs")
local plugins = prism.fs.plugins()

local docsByName = {}
local completions = {}
for _, entry in ipairs(docs) do
    docsByName[entry.name] = entry
    if not entry.method then
        completions[#completions + 1] = entry
    end
end

local function describe(entry)
    local text = entry.signature .. "\n\n" .. entry.doc
    if entry.permission then
        text = text .. "\n\nNeeds the " .. entry.permission .. " permission."
    end
    return text
end

-- The entries whose name or description contain the text, names first
local function search(text)
    text = text:lower()
    local byName, byDoc = {}, {}
    for _, entry in ipairs(docs) do
        if entry.name:lower():find(text, 1, true) then
            byName[#byName + 1] = entry.name
        elseif text ~= "" and entry.doc:lower():find(text, 1, true) then
            byDoc[#byDoc + 1] = entry.name
        end
    end
    for _, name in ipairs(byDoc) do
        byName[#byName + 1] = name
    end
    return byName
end

local editor = nil

-- Every .lua file of every plugin, as paths relative to the plugins folder
local function luaFiles()
    local files = {}
    local function walk(dir, depth)
        for _, entry in ipairs(plugins:list(dir)) do
            local path = dir == "" and entry.name or (dir .. "/" .. entry.name)
            if entry.name:sub(1, 1) ~= "." then
                if entry.dir and depth < 4 then
                    walk(path, depth + 1)
                elseif not entry.dir and path:match("%.lua$") then
                    files[#files + 1] = path
                end
            end
        end
    end
    walk("", 0)
    table.sort(files)
    return files
end

local function open()
    if editor then
        editor.window:show()
        return
    end

    -- note: a message shown until the file changes again, such as "Saved."
    local state = { files = {}, current = nil, saved = "", selecting = false, note = nil }
    local window, list, status, code

    local function modified()
        return state.current ~= nil and code:text() ~= state.saved
    end

    local function update()
        if not state.current then
            return
        end
        local problems = checks.run(code:text(), state.current)
        code:setProblems(problems)
        local summary
        if #problems == 0 then
            summary = "No problems."
        else
            local first = problems[1]
            summary = string.format("%d problem%s. Line %d: %s", #problems, #problems == 1 and "" or "s", first.line, first.message)
        end
        if modified() then
            state.note = nil
        end
        status:setText(state.current .. (modified() and " (modified)" or "") .. " - " .. (state.note and (state.note .. " ") or "") .. summary)
        window:setText("Lua editor - " .. state.current .. (modified() and " *" or ""))
    end

    local function refresh()
        state.files = luaFiles()
        state.selecting = true
        list:setItems(state.files)
        for i, file in ipairs(state.files) do
            if file == state.current then
                list:select(i)
            end
        end
        state.selecting = false
    end

    local function select(index, file)
        if state.selecting or file == state.current then
            return
        end
        if modified() then
            -- Go back to the open file instead of losing its changes
            state.selecting = true
            for i, other in ipairs(state.files) do
                if other == state.current then
                    list:select(i)
                end
            end
            state.selecting = false
            status:setText("Save or revert " .. state.current .. " before opening another file.")
            return
        end
        local text = plugins:read(file)
        state.current = file
        state.saved = text
        code:setEnabled(true)
        code:setText(text)
        update()
    end

    local function save()
        if not state.current then
            return
        end
        local text = code:text()
        local ok, err = pcall(plugins.write, plugins, state.current, text)
        if not ok then
            status:setText("Could not save " .. state.current .. ": " .. tostring(err))
            return
        end
        state.saved = text
        -- Shown in the window instead of a notification, which are limited per minute
        state.note = "Saved; disable and enable its plugin to apply the changes."
        update()
    end

    local function revert()
        if state.current then
            code:setText(state.saved)
            update()
        end
    end

    -- Documentation panel
    local docView, results

    local function showDoc(name)
        local entry = docsByName[name]
        -- Keep the last one while the cursor is on something undocumented
        if entry then
            docView:setText(describe(entry))
        end
    end

    local function filter(text)
        results:setItems(search(text))
    end

    window = prism.ui.window{ title = "Lua editor", width = 1300, height = 700, onClose = function() editor = nil end }
    local row = window:add("row", { stretch = 1 })
    local left = row:add("column", { stretch = 1 })
    list = left:add("list", { stretch = 1, onSelect = select })
    left:add("button", { text = "Refresh", onClick = refresh })
    local middle = row:add("column", { stretch = 3 })
    status = middle:add("label", { text = "Pick a file on the left." })
    code = middle:add("code", { stretch = 1, onChange = update, onFocus = showDoc })
    code:setCompletions(completions)
    code:setEnabled(false)
    local buttons = middle:add("row")
    buttons:add("button", { text = "Save", onClick = save })
    buttons:add("button", { text = "Revert", onClick = revert })
    local right = row:add("column", { stretch = 2 })
    right:add("label", { text = "Documentation: put the cursor on a name, or search (Ctrl+Space completes)." })
    local query
    query = right:add("input", { placeholder = "Search, e.g. http or string.f", onChange = function() filter(query:text()) end })
    results = right:add("list", { stretch = 1, onSelect = function(_, name) showDoc(name) end })
    docView = right:add("text", { stretch = 1, readOnly = true })
    filter("prism")

    refresh()
    editor = { window = window }
    window:show()
end

prism.ui.addAction{ text = "Lua editor", scope = "launcher", run = open }
