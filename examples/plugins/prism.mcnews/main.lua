-- Tells when a new Minecraft release or snapshot is out, checking Mojang's version list when the launcher starts.
local settings = prism.settings{ lastRelease = "", lastSnapshot = "", snapshots = true }
local versionsUrl = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"

-- manual: asked by the user, who gets an answer even if nothing is new
local function check(manual)
    prism.async(function()
        local response = prism.http.get(versionsUrl, { timeout = 20 })
        if not response.ok then
            prism.log.warn("Could not check for new Minecraft versions:", response.status, response.error)
            if manual then
                prism.ui.notify("Could not reach Mojang to check for new versions.", "warning")
            end
            return
        end
        local ok, manifest = pcall(prism.json.decode, response.body)
        if not ok or type(manifest) ~= "table" or type(manifest.latest) ~= "table" then
            prism.log.warn("Unexpected answer from Mojang's version list")
            return
        end

        local release, snapshot = manifest.latest.release, manifest.latest.snapshot
        local news = {}
        -- The first check only remembers the current versions
        if release ~= settings.lastRelease then
            if settings.lastRelease ~= "" then
                news[#news + 1] = "Minecraft " .. release .. " is out!"
            end
            settings.lastRelease = release
        end
        if settings.snapshots and snapshot ~= release and snapshot ~= settings.lastSnapshot then
            if settings.lastSnapshot ~= "" then
                news[#news + 1] = "New snapshot: " .. snapshot
            end
            settings.lastSnapshot = snapshot
        end

        for _, message in ipairs(news) do
            prism.ui.notify(message)
        end
        if manual and #news == 0 then
            prism.ui.notify("Latest Minecraft: " .. release .. " (snapshot " .. snapshot .. ")")
        end
    end)
end

prism.on("launcher.ready", function() check(false) end)
prism.ui.addAction{ text = "Check Minecraft versions", scope = "launcher", run = function() check(true) end }
