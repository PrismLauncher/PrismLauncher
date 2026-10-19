-- Checks the helpers in prelude.lua; run by PrismApi_test
local json = prism.json
local encoded = json.encode({ name = "Steve", level = 3, ratio = 0.5, tags = { "a", "b" }, ok = true, none = json.array() })
assert(encoded == [[{"level":3,"name":"Steve","none":[],"ok":true,"ratio":0.5,"tags":["a","b"]}]], encoded)
assert(json.encode("quote \" and \n") == [["quote \" and \n"]])
assert(json.encode({}) == "{}")
assert(json.encode({ 1, 2 }, { pretty = true }) == "[\n  1,\n  2\n]")

local decoded = json.decode([[ { "a": [1, 2.5, "x", true, null, {"b": {}}], "u": "\u00f1\ud83d\ude00", "e": 1e3 } ]])
assert(decoded.a[1] == 1 and math.type(decoded.a[1]) == "integer")
assert(decoded.a[2] == 2.5 and decoded.a[3] == "x" and decoded.a[4] == true and decoded.a[5] == nil)
assert(type(decoded.a[6].b) == "table")
assert(decoded.u == "\u{f1}\u{1F600}", decoded.u)
assert(decoded.e == 1000)
assert(json.encode(json.decode("[]")) == "[]")

for _, bad in ipairs({ "{", "[1,]", "{\"a\" 1}", "tru", "\"unterminated", "[1] 2" }) do
    local ok, err = pcall(json.decode, bad)
    assert(not ok and err:find("invalid JSON at position"), bad .. " -> " .. tostring(err))
end
local t = {}
t.self = t
assert(not pcall(json.encode, t))
assert(not pcall(json.encode, { f = print }))

local path = prism.path
assert(path.join("a", "b/", "/c.txt") == "a/b/c.txt")
assert(path.basename("saves/World/level.dat") == "level.dat")
assert(path.dirname("saves/World/level.dat") == "saves/World")
assert(path.extension("mod.jar") == ".jar" and path.extension("README") == "")
assert(path.stem("backup.tar.gz") == "backup.tar")

local text = prism.text
assert(text.trim("  hi \n") == "hi")
assert(text.startsWith("prism.log", "prism.") and text.endsWith("main.lua", ".lua"))
local parts = text.split("a,,b", ",")
assert(#parts == 3 and parts[2] == "")
assert(#text.split("a.b", ".") == 2)
local lines = text.lines("one\r\ntwo\n")
assert(#lines == 2 and lines[2] == "two")

local shown = prism.inspect({ b = 1, a = { "x" } })
assert(shown == "{ a = { [1] = \"x\" }, b = 1 }", shown)

