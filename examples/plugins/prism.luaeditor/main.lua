-- A small editor for the Lua files of the installed plugins, which points out problems while typing.
local checks = require("checks")
local plugins = prism.fs.plugins()

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

    local state = { files = {}, current = nil, saved = "", selecting = false }
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
        status:setText(state.current .. (modified() and " (modified)" or "") .. " - " .. summary)
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
        plugins:write(state.current, text)
        state.saved = text
        update()
        prism.ui.notify("Saved " .. state.current .. ". Disable and enable its plugin to apply the changes.")
    end

    local function revert()
        if state.current then
            code:setText(state.saved)
            update()
        end
    end

    window = prism.ui.window{ title = "Lua editor", width = 1000, height = 650, onClose = function() editor = nil end }
    local row = window:add("row", { stretch = 1 })
    local left = row:add("column", { stretch = 1 })
    list = left:add("list", { stretch = 1, onSelect = select })
    left:add("button", { text = "Refresh", onClick = refresh })
    local right = row:add("column", { stretch = 3 })
    status = right:add("label", { text = "Pick a file on the left." })
    code = right:add("code", { stretch = 1, onChange = update })
    code:setEnabled(false)
    local buttons = right:add("row")
    buttons:add("button", { text = "Save", onClick = save })
    buttons:add("button", { text = "Revert", onClick = revert })

    refresh()
    editor = { window = window }
    window:show()
end

prism.ui.addAction{ text = "Lua editor", scope = "launcher", run = open }
