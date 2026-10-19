-- Finds problems in Lua code without running it: syntax errors, and uses of prism.* names that do not exist.
local checks = {}

-- Every name reachable from the prism table, e.g. "prism.fs.data", mapped to its table (or true for leaves)
local api = {}
local function collect(value, name, seen)
    api[name] = type(value) == "table" and value or true
    if type(value) ~= "table" or seen[value] then
        return
    end
    seen[value] = true
    for key, child in pairs(value) do
        if type(key) == "string" then
            collect(child, name .. "." .. key, seen)
        end
    end
end
collect(prism, "prism", {})

-- How many single character edits turn a into b
local function distance(a, b)
    local previous = {}
    for j = 0, #b do
        previous[j] = j
    end
    for i = 1, #a do
        local current = { [0] = i }
        for j = 1, #b do
            local cost = a:sub(i, i) == b:sub(j, j) and 0 or 1
            current[j] = math.min(previous[j] + 1, current[j - 1] + 1, previous[j - 1] + cost)
        end
        previous = current
    end
    return previous[#b]
end

-- The closest existing field of the parent, if it is close enough to be a typo
local function suggestion(parent, field)
    local best, bestDistance = nil, 3
    local fields = api[parent]
    if type(fields) ~= "table" then
        return nil
    end
    for key in pairs(fields) do
        if type(key) == "string" then
            local d = distance(field, key)
            if d < bestDistance then
                best, bestDistance = key, d
            end
        end
    end
    return best and (parent .. "." .. best)
end

-- Replaces the contents of strings and comments with spaces, keeping line breaks, so names inside them are ignored
local function blankStringsAndComments(code)
    local out = {}
    local i, n = 1, #code
    local function blank(from, to)
        out[#out + 1] = code:sub(from, to):gsub("[^\n]", " ")
    end
    while i <= n do
        local c = code:sub(i, i)
        local long = code:match("^%[(=*)%[", i)
        if code:sub(i, i + 1) == "--" then
            local commentLong = code:match("^%-%-%[(=*)%[", i)
            local stop
            if commentLong then
                local _, e = code:find("]" .. commentLong .. "]", i, true)
                stop = e or n
            else
                stop = (code:find("\n", i, true) or n + 1) - 1
            end
            blank(i, stop)
            i = stop + 1
        elseif long then
            local _, e = code:find("]" .. long .. "]", i, true)
            local stop = e or n
            blank(i, stop)
            i = stop + 1
        elseif c == '"' or c == "'" then
            local j = i + 1
            while j <= n do
                local d = code:sub(j, j)
                if d == "\\" then
                    j = j + 2
                elseif d == c or d == "\n" then
                    break
                else
                    j = j + 1
                end
            end
            blank(i, math.min(j, n))
            i = j + 1
        else
            out[#out + 1] = c
            i = i + 1
        end
    end
    return table.concat(out)
end

-- Returns a list of { line = number, message = string }
function checks.run(code, chunkName)
    local problems = {}
    local _, err = load(code, "=" .. chunkName, "t")
    if err then
        local line, message = err:match(":(%d+): (.*)$")
        problems[#problems + 1] = { line = tonumber(line) or 1, message = message or err }
    end

    local lineNumber = 0
    for line in (blankStringsAndComments(code) .. "\n"):gmatch("([^\n]*)\n") do
        lineNumber = lineNumber + 1
        for match in line:gmatch("%f[%w_]prism[%.%w_]*") do
            -- In Lua 5.5 loop variables are constant, so the cleaned name needs its own
            local name = match:gsub("%.$", "")
            -- Check each step, so prism.fs.dta is reported even if more follows
            local known = "prism"
            for field in name:gmatch("%.([%w_]+)") do
                local next = known .. "." .. field
                if not api[next] then
                    if type(api[known]) == "table" then
                        local hint = suggestion(known, field)
                        problems[#problems + 1] = {
                            line = lineNumber,
                            message = "unknown API " .. next .. (hint and (", did you mean " .. hint .. "?") or ""),
                        }
                    end
                    break
                end
                known = next
            end
        end
    end

    table.sort(problems, function(a, b) return a.line < b.line end)
    return problems
end

return checks
