-- Backs up the worlds of an instance before every launch, keeping the last few, and restores them from a window.
local settings = prism.settings{ keep = 3 }
local data = prism.fs.data()

-- Backups live in data/<instance id>/<number>, the highest number being the newest
local function backups(instanceId)
    local numbers = {}
    if data:isDir(instanceId) then
        for _, entry in ipairs(data:list(instanceId)) do
            local number = tonumber(entry.name)
            if entry.dir and number then
                numbers[#numbers + 1] = number
            end
        end
    end
    table.sort(numbers)
    return numbers
end

local function backUp(ctx)
    local game = prism.fs.instance(ctx.instance.id)
    if not game:isDir("saves") or #game:list("saves") == 0 then
        ctx.log("No worlds to back up")
        return
    end
    local existing = backups(ctx.instance.id)
    local number = (existing[#existing] or 0) + 1
    prism.fs.copy(game, "saves", data, ctx.instance.id .. "/" .. number)
    -- Only the newest ones are kept
    for i = 1, #existing + 1 - settings.keep do
        data:remove(ctx.instance.id .. "/" .. existing[i], { recursive = true })
    end
    ctx.log("Worlds backed up (backup " .. number .. ")")
end

prism.launch.addStep{ before = "launch", name = "Back up worlds", run = backUp }

-- A window listing the backups of an instance, to restore a world from them
local function showBackups(ctx)
    local id, name = ctx.instance.id, ctx.instance.name
    local window = prism.ui.window{ title = "World backups - " .. name, width = 500, height = 400 }
    local status = window:add("label", { text = "Pick a world from a backup to restore it next to the current ones." })
    local list = window:add("list", { stretch = 1 })
    local entries = {}

    local function refresh()
        entries = {}
        local labels = {}
        local numbers = backups(id)
        -- Newest first
        for i = #numbers, 1, -1 do
            local folder = id .. "/" .. numbers[i]
            for _, world in ipairs(data:list(folder)) do
                if world.dir then
                    entries[#entries + 1] = { folder = folder, world = world.name, number = numbers[i] }
                    labels[#labels + 1] = "Backup " .. numbers[i] .. ": " .. world.name
                end
            end
        end
        list:setItems(labels)
        if #entries == 0 then
            status:setText("There are no backups yet; they are made before each launch.")
        end
    end

    local function restore()
        local entry = entries[list:selected() or 0]
        if not entry then
            status:setText("Pick a world first.")
            return
        end
        local game = prism.fs.instance(id)
        -- Never overwrites: the restored world gets a new name
        local target = "saves/" .. entry.world .. " (backup " .. entry.number .. ")"
        if game:exists(target) then
            status:setText(target .. " already exists; rename or remove it first.")
            return
        end
        prism.fs.copy(data, entry.folder .. "/" .. entry.world, game, target)
        status:setText("Restored as \"" .. prism.path.basename(target) .. "\".")
    end

    local buttons = window:add("row")
    buttons:add("button", { text = "Restore", onClick = restore })
    buttons:add("button", { text = "Refresh", onClick = refresh })
    refresh()
    window:show()
end

prism.ui.addAction{ text = "World backups", scope = "instance", run = showBackups }
