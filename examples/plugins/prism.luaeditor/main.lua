-- A small editor for the Lua files of the installed plugins, which points out problems while typing, completes names
-- and shows the documentation of the prism API and of Lua.
local checks = require("checks")
local docs = require("docs")
local plugins = prism.fs.plugins()

-- The editor's layout, remembered between uses; panes are the sizes of the file list, the code and the help
local layout = prism.settings{ width = 1300, height = 700, showHelp = true, panes = "", plugin = "" }

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

-- Files the editor opens: code, the manifest and plain text
local function isEditable(name)
    return isLua(name) or name == "plugin.toml" or name:match("%.json$") or name:match("%.md$") or name:match("%.txt$")
end

-- The folders of the installed plugins
local function pluginFolders()
    local folders = {}
    for _, entry in ipairs(plugins:list()) do
        if entry.dir and entry.name:sub(1, 1) ~= "." then
            folders[#folders + 1] = entry.name
        end
    end
    table.sort(folders)
    return folders
end

-- Tree items for the editable files of a plugin, with its folders; ids are paths inside the plugins folder
local function fileTree(plugin)
    local function walk(dir, depth)
        local items = {}
        for _, entry in ipairs(plugins:list(dir)) do
            local path = dir .. "/" .. entry.name
            if entry.name:sub(1, 1) ~= "." then
                if entry.dir and depth < 8 then
                    local children = walk(path, depth + 1)
                    -- Folders without anything to edit are left out
                    if #children > 0 then
                        items[#items + 1] = { text = entry.name, children = children }
                    end
                elseif not entry.dir and isEditable(entry.name) then
                    items[#items + 1] = { text = entry.name, id = path }
                end
            end
        end
        return items
    end
    return walk(plugin, 1)
end

local function open()
    if editor then
        editor.window:show()
        return
    end

    -- note: a message shown until the file changes again, such as "Saved."
    -- plugin: the folder being edited; current: the open file, as a path inside the plugins folder
    local state = { folders = {}, plugin = nil, current = nil, saved = "", selecting = false, note = nil }
    local window, pluginPicker, tree, status, code

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

    -- Runs fn without reacting to the selection changes it causes
    local function quietly(fn)
        state.selecting = true
        fn()
        state.selecting = false
    end

    local function refreshTree()
        quietly(function()
            tree:setItems(state.plugin and fileTree(state.plugin) or {})
            if state.current then
                tree:select(state.current)
            end
        end)
    end

    local function selectPluginInPicker()
        quietly(function()
            for i, folder in ipairs(state.folders) do
                if folder == state.plugin then
                    pluginPicker:select(i)
                end
            end
        end)
    end

    local function refresh()
        state.folders = pluginFolders()
        quietly(function() pluginPicker:setItems(state.folders) end)
        selectPluginInPicker()
        refreshTree()
    end

    local function closeFile()
        state.current = nil
        state.saved = ""
        quietly(function() code:setText("") end)
        code:setProblems({})
        code:setEnabled(false)
        status:setText("Pick a file of " .. (state.plugin or "a plugin") .. ".")
        window:setText("Lua editor")
    end

    local function choosePlugin(_, folder)
        if state.selecting or folder == state.plugin then
            return
        end
        if modified() then
            -- Go back instead of losing the changes
            selectPluginInPicker()
            status:setText("Save or revert " .. state.current .. " before opening another plugin.")
            return
        end
        state.plugin = folder
        layout.plugin = folder
        closeFile()
        refreshTree()
    end

    local function select(file)
        if state.selecting or file == "" or file == state.current then
            return
        end
        if modified() then
            -- Go back to the open file instead of losing its changes
            quietly(function() tree:select(state.current) end)
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
    left:add("label", { text = "Plugin to edit:" })
    pluginPicker = left:add("combo", { onSelect = choosePlugin })
    tree = left:add("tree", { stretch = 1, onSelect = select })
    left:add("button", { text = "Refresh", onClick = refresh })

    local middle = panes:add("column", { stretch = 3 })
    status = middle:add("label", { text = "Pick a plugin and one of its files on the left." })
    code = middle:add("code", { stretch = 1, onChange = update, onFocus = showDoc })
    code:setCompletions(completions)
    code:setEnabled(false)
    local buttons = middle:add("row")
    buttons:add("button", { text = "Save (Ctrl+S)", onClick = save })
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

    -- Start with the plugin edited last time, or the first one
    state.folders = pluginFolders()
    state.plugin = state.folders[1]
    for _, folder in ipairs(state.folders) do
        if folder == layout.plugin then
            state.plugin = folder
        end
    end
    window:addShortcut("Ctrl+S", save)

    refresh()
    editor = { window = window }
    window:show()
    showHelp(layout.showHelp)
end

prism.ui.addAction{ text = "Lua editor", scope = "launcher", run = open }
