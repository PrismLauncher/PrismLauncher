-- A small editor for the Lua files of the installed plugins, which points out problems while typing, completes names
-- and shows the documentation of the prism API and of Lua.
local checks = require("checks")
local docs = require("docs")
local plugins = prism.fs.plugins()

-- The editor's layout, remembered between uses; panes are the sizes of the file list, the code and the help
local layout = prism.settings{ width = 1300, height = 700, showHelp = true, panes = "" }

local function parseSizes(text)
    local sizes = {}
    for size in text:gmatch("%d+") do
        sizes[#sizes + 1] = tonumber(size)
    end
    return #sizes == 3 and sizes or nil
end

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

local function isLua(path)
    return path:match("%.lua$") ~= nil
end

-- Every .lua file and plugin.toml of every plugin, as paths relative to the plugins folder
local function pluginFiles()
    local files = {}
    local function walk(dir, depth)
        for _, entry in ipairs(plugins:list(dir)) do
            local path = dir == "" and entry.name or (dir .. "/" .. entry.name)
            if entry.name:sub(1, 1) ~= "." then
                if entry.dir and depth < 4 then
                    walk(path, depth + 1)
                elseif not entry.dir and (isLua(path) or entry.name == "plugin.toml") then
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
        -- The checks are for Lua; manifests are plain TOML
        local problems = isLua(state.current) and checks.run(code:text(), state.current) or {}
        code:setProblems(problems)
        local summary
        if not isLua(state.current) then
            summary = "Changes to permissions need approval again in Settings > Plugins."
        elseif #problems == 0 then
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
        state.files = pluginFiles()
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

    local panes, right, helpButton

    local function rememberPanes()
        -- Only while the help is shown: hidden, its pane measures nothing
        if layout.showHelp then
            layout.panes = table.concat(panes:sizes(), ",")
        end
    end

    local function showHelp(visible)
        right:setVisible(visible)
        helpButton:setText(visible and "Hide help" or "Show help")
        local sizes = visible and parseSizes(layout.panes)
        if sizes then
            panes:setSizes(sizes)
        end
    end

    local function toggleHelp()
        rememberPanes()
        layout.showHelp = not layout.showHelp
        showHelp(layout.showHelp)
    end

    window = prism.ui.window{ title = "Lua editor", width = layout.width, height = layout.height, onClose = function()
        layout.width, layout.height = window:size()
        rememberPanes()
        editor = nil
    end }

    -- Three panes the user resizes by dragging the handles between them
    panes = window:add("split", { stretch = 1 })
    local left = panes:add("column", { stretch = 1 })
    list = left:add("list", { stretch = 1, onSelect = select })
    left:add("button", { text = "Refresh", onClick = refresh })

    local middle = panes:add("column", { stretch = 3 })
    status = middle:add("label", { text = "Pick a file on the left." })
    code = middle:add("code", { stretch = 1, onChange = update, onFocus = showDoc })
    code:setCompletions(completions)
    code:setEnabled(false)
    local buttons = middle:add("row")
    buttons:add("button", { text = "Save", onClick = save })
    buttons:add("button", { text = "Revert", onClick = revert })
    helpButton = buttons:add("button", { text = "Hide help", onClick = toggleHelp })

    right = panes:add("column", { stretch = 2 })
    right:add("label", { text = "Documentation: put the cursor on a name, or search (Ctrl+Space completes)." })
    local query
    query = right:add("input", { placeholder = "Search, e.g. http or string.f", onChange = function() filter(query:text()) end })
    -- The results and the documentation share the height, also resizable
    local help = right:add("split", { vertical = true, stretch = 1 })
    results = help:add("list", { stretch = 1, onSelect = function(_, name) showDoc(name) end })
    docView = help:add("text", { stretch = 1, readOnly = true })
    filter("prism")

    refresh()
    editor = { window = window }
    window:show()
    showHelp(layout.showHelp)
end

prism.ui.addAction{ text = "Lua editor", scope = "launcher", run = open }
