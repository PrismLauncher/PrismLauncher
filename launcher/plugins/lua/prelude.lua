-- SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
--
-- SPDX-License-Identifier: GPL-3.0-only

-- Helpers added to the prism table of every plugin. Runs with the same sandbox and limits as the plugin.
local prism = ...

---------------------------------------------------------------------------------------------------- prism.json

local json = {}
prism.json = json

-- Marks a table to be encoded as an array even when empty
local arrayMarker = setmetatable({}, { __mode = "k" })
function json.array(t)
    t = t or {}
    arrayMarker[t] = true
    return t
end

local escapes = { ['"'] = '\\"', ["\\"] = "\\\\", ["\b"] = "\\b", ["\f"] = "\\f", ["\n"] = "\\n", ["\r"] = "\\r", ["\t"] = "\\t" }

local function encodeString(s)
    return '"' .. s:gsub('[%c"\\]', function(c)
        return escapes[c] or string.format("\\u%04x", c:byte())
    end) .. '"'
end

local function isArray(t)
    if arrayMarker[t] then
        return true
    end
    local n = #t
    if n == 0 then
        return false
    end
    local count = 0
    for k in pairs(t) do
        if math.type(k) ~= "integer" or k < 1 or k > n then
            return false
        end
        count = count + 1
    end
    return count == n
end

local function encode(value, indent, depth, seen)
    local kind = type(value)
    if value == nil or value == json.null then
        return "null"
    elseif kind == "boolean" then
        return tostring(value)
    elseif kind == "number" then
        if value ~= value or value == math.huge or value == -math.huge then
            error("cannot encode " .. tostring(value) .. " as JSON", 0)
        end
        if math.type(value) == "integer" then
            return tostring(value)
        end
        return string.format("%.17g", value)
    elseif kind == "string" then
        return encodeString(value)
    elseif kind ~= "table" then
        error("cannot encode a " .. kind .. " as JSON", 0)
    end

    if seen[value] then
        error("cannot encode a table that contains itself", 0)
    end
    seen[value] = true
    local newline = indent and "\n" .. string.rep(indent, depth + 1) or ""
    local close = indent and "\n" .. string.rep(indent, depth) or ""
    local separator = indent and ": " or ":"
    local parts = {}
    local result
    if isArray(value) then
        for i = 1, #value do
            parts[i] = newline .. encode(value[i], indent, depth + 1, seen)
        end
        result = #parts == 0 and "[]" or "[" .. table.concat(parts, ",") .. close .. "]"
    else
        local keys = {}
        for k in pairs(value) do
            if type(k) ~= "string" and type(k) ~= "number" then
                error("cannot encode a table with " .. type(k) .. " keys as JSON", 0)
            end
            keys[#keys + 1] = tostring(k)
        end
        table.sort(keys)
        for i, k in ipairs(keys) do
            local v = value[k]
            if v == nil then
                v = value[tonumber(k)]
            end
            parts[i] = newline .. encodeString(k) .. separator .. encode(v, indent, depth + 1, seen)
        end
        result = #parts == 0 and "{}" or "{" .. table.concat(parts, ",") .. close .. "}"
    end
    seen[value] = nil
    return result
end

-- Sentinel for JSON null, when it must be kept (decoded nulls become nil)
json.null = setmetatable({}, { __tostring = function() return "null" end })

function json.encode(value, opts)
    local indent = opts and opts.pretty and "  " or nil
    return encode(value, indent, 0, {})
end

local function decodeError(text, pos, message)
    error(string.format("invalid JSON at position %d: %s", pos, message), 0)
end

local function skip(text, pos)
    return text:find("[^ \t\r\n]", pos) or #text + 1
end

local decodeValue

local function decodeString(text, pos)
    local parts = {}
    local i = pos + 1
    while true do
        local start, stop, piece, special = text:find('^([^"\\]*)(["\\])', i)
        if not start then
            decodeError(text, pos, "unterminated string")
        end
        parts[#parts + 1] = piece
        i = stop + 1
        if special == '"' then
            return table.concat(parts), i
        end
        local escape = text:sub(i, i)
        local simple = { ['"'] = '"', ["\\"] = "\\", ["/"] = "/", b = "\b", f = "\f", n = "\n", r = "\r", t = "\t" }
        if simple[escape] then
            parts[#parts + 1] = simple[escape]
            i = i + 1
        elseif escape == "u" then
            local hex = text:match("^%x%x%x%x", i + 1)
            if not hex then
                decodeError(text, i, "invalid unicode escape")
            end
            local code = tonumber(hex, 16)
            i = i + 5
            -- Surrogate pairs
            if code >= 0xD800 and code <= 0xDBFF then
                local low = text:match("^\\u(%x%x%x%x)", i)
                if low then
                    code = 0x10000 + (code - 0xD800) * 0x400 + (tonumber(low, 16) - 0xDC00)
                    i = i + 6
                end
            end
            parts[#parts + 1] = utf8.char(code)
        else
            decodeError(text, i, "invalid escape")
        end
    end
end

function decodeValue(text, pos, depth)
    if depth > 200 then
        decodeError(text, pos, "too deeply nested")
    end
    pos = skip(text, pos)
    local c = text:sub(pos, pos)
    if c == "{" then
        local result = {}
        pos = skip(text, pos + 1)
        if text:sub(pos, pos) == "}" then
            return result, pos + 1
        end
        while true do
            if text:sub(pos, pos) ~= '"' then
                decodeError(text, pos, "expected a key")
            end
            local key
            key, pos = decodeString(text, pos)
            pos = skip(text, pos)
            if text:sub(pos, pos) ~= ":" then
                decodeError(text, pos, "expected ':'")
            end
            result[key], pos = decodeValue(text, pos + 1, depth + 1)
            pos = skip(text, pos)
            local next = text:sub(pos, pos)
            if next == "}" then
                return result, pos + 1
            elseif next ~= "," then
                decodeError(text, pos, "expected ',' or '}'")
            end
            pos = skip(text, pos + 1)
        end
    elseif c == "[" then
        local result = json.array()
        local n = 0
        pos = skip(text, pos + 1)
        if text:sub(pos, pos) == "]" then
            return result, pos + 1
        end
        while true do
            n = n + 1
            result[n], pos = decodeValue(text, pos, depth + 1)
            pos = skip(text, pos)
            local next = text:sub(pos, pos)
            if next == "]" then
                return result, pos + 1
            elseif next ~= "," then
                decodeError(text, pos, "expected ',' or ']'")
            end
            pos = pos + 1
        end
    elseif c == '"' then
        return decodeString(text, pos)
    elseif text:find("^true", pos) then
        return true, pos + 4
    elseif text:find("^false", pos) then
        return false, pos + 5
    elseif text:find("^null", pos) then
        return nil, pos + 4
    else
        local number = text:match("^-?%d+%.?%d*[eE]?[+-]?%d*", pos)
        if not number or number == "" or number == "-" then
            decodeError(text, pos, "unexpected '" .. c .. "'")
        end
        local value = tonumber(number)
        if not value then
            decodeError(text, pos, "invalid number")
        end
        return math.tointeger(value) or value, pos + #number
    end
end

function json.decode(text)
    if type(text) ~= "string" then
        error("bad argument #1 to 'decode' (string expected, got " .. type(text) .. ")", 2)
    end
    local value, pos = decodeValue(text, 1, 0)
    pos = skip(text, pos)
    if pos <= #text then
        decodeError(text, pos, "unexpected trailing characters")
    end
    return value
end

---------------------------------------------------------------------------------------------------- prism.path

local path = {}
prism.path = path

-- Joins the parts with "/", without doubling separators
function path.join(...)
    local parts = {}
    for _, part in ipairs({ ... }) do
        if part ~= "" then
            parts[#parts + 1] = part
        end
    end
    return (table.concat(parts, "/"):gsub("/+", "/"))
end

function path.basename(p)
    return (p:gsub("/+$", ""):match("[^/]*$"))
end

function path.dirname(p)
    local dir = p:gsub("/+$", ""):match("^(.*)/[^/]*$")
    return dir or ""
end

function path.extension(p)
    return path.basename(p):match("^.+(%.[^.]+)$") or ""
end

function path.stem(p)
    local base = path.basename(p)
    return base:match("^(.+)%.[^.]+$") or base
end

---------------------------------------------------------------------------------------------------- prism.text

local text = {}
prism.text = text

function text.trim(s)
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

function text.startsWith(s, prefix)
    return s:sub(1, #prefix) == prefix
end

function text.endsWith(s, suffix)
    return suffix == "" or s:sub(-#suffix) == suffix
end

-- Splits on a plain separator (not a pattern); keeps empty pieces
function text.split(s, separator)
    if separator == nil or separator == "" then
        error("bad argument #2 to 'split' (non-empty separator expected)", 2)
    end
    local result = {}
    local start = 1
    while true do
        local i, j = s:find(separator, start, true)
        if not i then
            result[#result + 1] = s:sub(start)
            return result
        end
        result[#result + 1] = s:sub(start, i - 1)
        start = j + 1
    end
end

function text.lines(s)
    local result = text.split((s:gsub("\r\n", "\n")), "\n")
    if result[#result] == "" then
        result[#result] = nil
    end
    return result
end

---------------------------------------------------------------------------------------------------- prism.inspect

-- A readable representation of any value, for logging
function prism.inspect(value, depth)
    depth = depth or 0
    local kind = type(value)
    if kind == "string" then
        return string.format("%q", value)
    elseif kind ~= "table" then
        return tostring(value)
    elseif depth > 5 then
        return "{...}"
    end
    local keys = {}
    for k in pairs(value) do
        keys[#keys + 1] = k
    end
    table.sort(keys, function(a, b)
        if type(a) == type(b) and (type(a) == "number" or type(a) == "string") then
            return a < b
        end
        return type(a) < type(b)
    end)
    local parts = {}
    for _, k in ipairs(keys) do
        local key = type(k) == "string" and k:match("^[%a_][%w_]*$") and k or "[" .. prism.inspect(k, depth + 1) .. "]"
        parts[#parts + 1] = key .. " = " .. prism.inspect(value[k], depth + 1)
    end
    return "{ " .. table.concat(parts, ", ") .. " }"
end
