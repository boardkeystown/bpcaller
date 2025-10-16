-- =========================================================
--  Cell
-- =========================================================
local Cell = {}
Cell.__index = Cell

function Cell:new(value, default)
    local o = {
        v = value or 0.0,
        d = default or 0.0,
        use_default = true,
        immutable = false
    }
    setmetatable(o, self)
    return o
end

function Cell:get_value()
    if not self.immutable and self.use_default then
        local v = self.v
        self.v = self.d
        return v
    end
    return self.v
end

function Cell:set_value(v)
    if not self.immutable then
        self.v = v
    end
end

function Cell:get_default()
    return self.d
end

function Cell:set_default(v)
    self.d = v
end

function Cell:copy()
    local o = Cell:new(self.v, self.d)
    o.use_default = self.use_default
    o.immutable = self.immutable
    return o
end

-- =========================================================
--  CellWrapper
-- =========================================================
local CellWrapper = {}
CellWrapper.__index = CellWrapper

function CellWrapper:new(value, default)
    local o = { cell = Cell:new(value, default) }
    setmetatable(o, self)
    return o
end

function CellWrapper:get_value()
    return self.cell:get_value()
end
function CellWrapper:set_value(v)
    self.cell:set_value(v)
end

function CellWrapper:get_default()
    return self.cell:get_default()
end
function CellWrapper:set_default(v)
    self.cell:set_default(v)
end

function CellWrapper:get_use_default()
    return self.cell.use_default
end
function CellWrapper:set_use_default(v)
    self.cell.use_default = v
end

function CellWrapper:get_immutable()
    return self.cell.immutable
end
function CellWrapper:set_immutable(v)
    self.cell.immutable = v
end

function CellWrapper:entangle(o)
    if getmetatable(o) == CellWrapper then
        self.cell = o.cell
    elseif getmetatable(o) == Cell then
        self.cell = o
    else
        error("object cannot be entangled")
    end
end

function CellWrapper:untangle()
    self.cell = self.cell:copy()
end

function CellWrapper:copy()
    local o = CellWrapper:new()
    o.cell = self.cell:copy()
    return o
end

-- =========================================================
--  Row
-- =========================================================
local Row = {}
Row.__index = function(t, k)
    if Row[k] then return Row[k] end
    local cells = rawget(t, "cells")
    local c = cells and cells[k]
    if c then
        return c:get_value()
    end
    return rawget(t, k)
end

Row.__newindex = function(t, k, v)
    local cells = rawget(t, "cells")
    if cells and cells[k] then
        cells[k]:set_value(v)
    else
        rawset(t, k, v)
    end
end

function Row:new(headers)
    local o = {}
    o.cells = {}
    for _, key in ipairs(headers) do
        o.cells[key] = CellWrapper:new()
    end
    setmetatable(o, self)
    return o
end

function Row:keys()
    local ks = {}
    for k,_ in pairs(self.cells) do
        table.insert(ks, k)
    end
    return ks
end

function Row:values()
    local vs = {}
    for _,v in pairs(self.cells) do
        table.insert(vs, v)
    end
    return vs
end

function Row:copy()
    local new_row = Row:new(self:keys())
    for k,v in pairs(self.cells) do
        new_row.cells[k] = v:copy()
    end
    return new_row
end

function Row:getCell(key)
    return self.cells[key]
end

function Row:setDefault(key, default, use_default)
    use_default = use_default == nil and true or use_default
    if key then
        local c = self.cells[key]
        if not c then return end
        if default ~= nil then c:set_default(default) end
        c:set_use_default(use_default)
    else
        for _, c in pairs(self.cells) do
            if default ~= nil then c:set_default(default) end
            c:set_use_default(use_default)
        end
    end
end

function Row:setImmutable(key, immutable)
    local c = self.cells[key]
    if c then c:set_immutable(immutable) end
end

function Row:__tostring()
    local parts = {}
    for k, v in pairs(self.cells) do
        table.insert(parts, string.format("%s = %s", k, tostring(v.cell.v)))
    end
    return string.format("Row( %s )", table.concat(parts, ", "))
end

-- =========================================================
--  Backend
-- =========================================================
local Backend = {}
Backend.__index = Backend

function Backend:new(filename)
    local f = assert(io.open(filename, "w"))
    local o = {
        file = f,
        isWriting = false,
        first_write = false,
        name = filename
    }
    setmetatable(o, self)
    return o
end

function Backend:w_header(list)
    if not self.isWriting then
        local nl = self.first_write and "\n" or ""
        self.first_write = true
        self.file:write(string.format("%s%s", nl, table.concat(list, ",")))
    else
        error(string.format("file '%s' started writing rows", self.name))
    end
end

function Backend:w_row(rows)
    self.isWriting = true
    local str_rows = {}
    for _,v in ipairs(rows) do
        table.insert(str_rows, tostring(v))
    end
    self.file:write("\n" .. table.concat(str_rows, ","))
end

function Backend:close()
    if self.file then
        self.file:close()
        self.file = nil
    end
end

-- =========================================================
--  Record
-- =========================================================
local Record = {}
Record.__index = Record

function Record:new(headers, backends, index)
    local o = {
        row = Row:new(headers),
        _index = index,
        _backends = backends
    }
    setmetatable(o, self)
    o:_w_header()
    return o
end

function Record:_w_header()
    for _, b in ipairs(self._backends) do
        if self._index == nil then
            b:w_header(self.row:keys())
        else
            local hdr = { "index" }
            for _,k in ipairs(self.row:keys()) do table.insert(hdr, k) end
            b:w_header(hdr)
        end
    end
end

function Record:_w_row()
    for _, b in ipairs(self._backends) do
        local row_vals = {}
        for _,v in ipairs(self.row:values()) do
            table.insert(row_vals, v:get_value())
        end
        if self._index ~= nil then
            table.insert(row_vals, 1, self._index)
        end
        b:w_row(row_vals)
    end
end

function Record:write(...)
    local args = {...}
    local nargs = select("#", ...)
    local first_arg = args[1]
    local named = type(first_arg) == "table" and not self.row.cells[first_arg]  -- detect keyword style
    if not named then
        local keys = self.row:keys()
        for i = 1, math.min(#args, #keys) do
            self.row[keys[i]] = args[i]
        end
    else
        for k,v in pairs(first_arg) do
            local c = self.row:getCell(k)
            if not c then error("Unknown key '" .. k .. "'") end
            c:set_value(v)
        end
    end
    self:_w_row()
end

-- =========================================================
--  Example usage
-- =========================================================
local backend = Backend:new("data.csv")
local record = Record:new({ "a", "b", "c" }, { backend }, 1)

record:write(123, 456, 789)
record:write({ a = 10, c = 99 })

local a = record.row:getCell('a')
a:set_value(123)

record:write()


backend:close()

print(record.row)  -- shows Row( a = 10, b = 456, c = 99 )


local Dog = {}
Dog.__index = function(t, k)
    if k == "name" then
        return rawget(t, "_name")    -- getter logic
    else
        return Dog[k]                -- fallback to methods
    end
end

Dog.__newindex = function(t, k, v)
    if k == "name" then
        rawset(t, "_name", v)        -- setter logic
    else
        rawset(t, k, v)
    end
end

function Dog:new(name)
    return setmetatable({ _name = name }, self)
end

local d = Dog:new("Rex")

print(d.name)  --> Rex
d.name = "Buddy"
print(d.name)  --> Buddy