-- dwm config, reloaded automatically every time this file is saved.
-- If it fails to load, dwm keeps the previous config and shows the error
-- in the bar until it is fixed.
--
-- dwm.key(mods, key, action)
--   mods    "super", "super+shift", "ctrl+alt", ... or "" for none
--   key     X keysym name: "q", "Return", "space", "comma", "XF86AudioMute"
--   action  a shell command string, an argv table or a Lua function

local key = dwm.key

dwm.input {
	mouse_accel  = false, -- flat profile, no pointer acceleration
	repeat_delay = 200,   -- ms before a held key starts repeating
	repeat_rate  = 35,    -- repeats per second
}

-- apps
key("super", "q", "ghostty")
key("super", "x", "helium-browser")
key("super", "e", "nautilus")
key("super", "p", "dmenu_run")

-- windows
key("super", "c", dwm.killclient)
key("super", "f", dwm.togglefullscreen)
key("super+shift", "space", dwm.togglefloating)
key("super", "Return", dwm.zoom)
key("super", "j", function() dwm.focusstack(1) end)
key("super", "k", function() dwm.focusstack(-1) end)

-- layout
key("super", "b", dwm.togglebar)
key("super", "i", function() dwm.incnmaster(1) end)
key("super", "d", function() dwm.incnmaster(-1) end)
key("super", "h", function() dwm.setmfact(-0.05) end)
key("super", "l", function() dwm.setmfact(0.05) end)
key("super", "t", function() dwm.setlayout(1) end)       -- tile
key("super+shift", "f", function() dwm.setlayout(2) end) -- floating
key("super", "m", function() dwm.setlayout(3) end)       -- monocle
key("super", "space", function() dwm.setlayout() end)    -- previous layout

-- monitors
key("super", "comma", function() dwm.focusmon(-1) end)
key("super", "period", function() dwm.focusmon(1) end)
key("super+shift", "comma", function() dwm.tagmon(-1) end)
key("super+shift", "period", function() dwm.tagmon(1) end)

-- tags
key("super", "Tab", function() dwm.view() end) -- previous tags
key("super", "0", function() dwm.view(0) end)  -- all tags
key("super+shift", "0", function() dwm.tag(0) end)
for i = 1, dwm.ntags do
	key("super", i, function() dwm.view(i) end)
	key("super+ctrl", i, function() dwm.toggleview(i) end)
	key("super+shift", i, function() dwm.tag(i) end)
	key("super+ctrl+shift", i, function() dwm.toggletag(i) end)
end

-- dwm
key("super+shift", "r", dwm.reload) -- saving this file already reloads it
key("super+shift", "q", dwm.quit)
