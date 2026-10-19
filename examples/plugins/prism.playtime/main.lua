-- Counts the play sessions of each instance and shows them with the time played.
local data = prism.fs.data()

-- Sessions per instance id, kept as JSON in the plugin's data folder
local sessions = {}
if data:exists("sessions.json") then
    local ok, saved = pcall(prism.json.decode, data:read("sessions.json"))
    if ok and type(saved) == "table" then
        sessions = saved
    end
end

local function save()
    data:write("sessions.json", prism.json.encode(sessions, { pretty = true }))
end

prism.on("launch.finished", function(e)
    if e.success then
        sessions[e.id] = (sessions[e.id] or 0) + 1
        save()
    end
end)

prism.on("instance.removed", function(e)
    if sessions[e.id] then
        sessions[e.id] = nil
        save()
    end
end)

local function duration(seconds)
    return string.format("%dh %02dm", seconds // 3600, seconds % 3600 // 60)
end

local function showPlayTime()
    local instances = prism.instances.list()
    table.sort(instances, function(a, b) return a.playTime > b.playTime end)
    local lines, total = {}, 0
    for _, instance in ipairs(instances) do
        total = total + instance.playTime
        local count = sessions[instance.id] or 0
        lines[#lines + 1] = string.format("%s - %s - %d session%s", instance.name, duration(instance.playTime), count, count == 1 and "" or "s")
    end

    local window = prism.ui.window{ title = "Play time", width = 450, height = 400 }
    window:add("label", { text = "Total: " .. duration(total) .. " in " .. #instances .. " instances" })
    window:add("list", { items = lines, stretch = 1 })
    window:show()
end

prism.ui.addAction{ text = "Play time", scope = "launcher", run = showPlayTime }
