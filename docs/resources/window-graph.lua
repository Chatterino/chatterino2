-- SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
--
-- SPDX-License-Identifier: CC0-1.0

--[=[

Dumps the window tree in a Chatterino instance to GraphViz.

Usage: /windows

]=]
--

local function enum_to_string(enum)
	local rev = {}
	for k, v in pairs(enum) do
		rev[v] = k
	end
	return function(v)
		local r = rev[v]
		if r ~= nil then
			return r
		end
		return "unknown-value(" .. v .. ")"
	end
end
local SPCNT_to_string = enum_to_string(c2.SplitContainerNodeType)
local WindowType_to_string = enum_to_string(c2.WindowType)

--- Slightly changed version of 11.6 String Buffers: https://www.lua.org/pil/11.6.html
local function new_strbuf()
	local s = { "" }
	---@param ... string
	function s:push(...)
		local size = #self
		for i, v in ipairs({ ... }) do
			self[size + i] = v
		end
		for i = #self - 1, 1, -1 do
			if string.len(self[i]) > string.len(self[i + 1]) then
				break
			end
			self[i] = self[i] .. table.remove(self)
		end
	end
	function s:flush()
		return table.concat(self)
	end
	return s
end
local outbuf = new_strbuf()

---@param s c2.Split|nil
---@param id string
local function do_split(s, id)
	if not s then
		outbuf:push(id, '[label="(nil)"]')
		return
	end
	outbuf:push(id, '[label="Split(channel=', s.channel:get_name(), ')"];\n')
end

---@param p c2.SplitContainerNode|nil
---@param id string
local function do_node(p, id)
	if not p then
		outbuf:push(id, '[label="(nil)"]')
		return
	end

	outbuf:push(
		id,
		'[label="SplitContainerNode(type=',
		SPCNT_to_string(p.type),
		", hflex=",
		string.format("%.2f", p.horizontal_flex),
		", vflex=",
		string.format("%.2f", p.vertical_flex),
		')"];\n'
	)
	if p.split then
		local sid = id .. "s"
		do_split(p.split, sid)
		outbuf:push(id, "->", sid, "\n")
	end
	for i, child in ipairs(p:children()) do
		local inner_id = id .. "c" .. i
		do_node(child, inner_id)
		outbuf:push(id, "->", inner_id, "\n")
	end
end

---@param p c2.SplitContainer|nil
---@param id string
local function do_container(p, id)
	if not p then
		outbuf:push(id, '[label="(nil)"]')
		return
	end
	outbuf:push(id, '[label="SplitContainer"];\n')
	local base_id = id .. "b"
	do_node(p.base_node, base_id)
	outbuf:push(id, "->", base_id, "\n")
end

---@param win c2.Window
---@param id string
local function do_window(win, id)
	outbuf:push(id, '[label="Window(type=', WindowType_to_string(win.type), ')"];\n')
	for i = 1, win.notebook.page_count do
		local inner_id = id .. "p" .. tostring(i)
		do_container(win.notebook:page_at(i - 1), inner_id)
		outbuf:push(id, "->", inner_id, "\n")
	end
end

c2.register_command("/windows", function(ctx)
	outbuf = new_strbuf()
	outbuf:push("digraph {\nrankdir=TB;\nnode[shape=rect];\n")
	for i, win in ipairs(c2.windows:all()) do
		local id = "w" .. i
		do_window(win, id)
	end
	outbuf:push("}\n")
	local s = outbuf:flush()
	local msg = c2.Message.new({
		elements = {
			{
				type = "text",
				color = "link",
				link = { type = c2.LinkType.CopyToClipboard, value = s },
				text = "Copy",
			},
		},
	})
	ctx.channel:add_message(msg)
end)
