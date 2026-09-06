.pragma library

var tagCount = 10;
var tagMask = (1 << tagCount) - 1;

function integer(value) {
    return typeof value === "number" && isFinite(value) && Math.floor(value) === value;
}

function mask(value) {
    return integer(value) && value >= 0 && value <= tagMask;
}

function decode(line) {
    var message = JSON.parse(line);
    if (!message || typeof message !== "object")
        throw new Error("Expected a JSON envelope");
    if (message.type === "state") {
        if (message.data === null)
            return message;
        var state = message.data;
        if (!state || !Array.isArray(state.tags) || !Array.isArray(state.monitors)
                || !integer(state.selectedMonitor))
            throw new Error("Invalid dwm state");
        var seen = {};
        for (var i = 0; i < state.monitors.length; ++i) {
            var m = state.monitors[i];
            if (!m || !integer(m.num) || m.num < 0 || seen[m.num]
                    || !integer(m.x) || !integer(m.y)
                    || !integer(m.width) || m.width <= 0
                    || !integer(m.height) || m.height <= 0
                    || !mask(m.selected) || !mask(m.occupied) || !mask(m.urgent)
                    || typeof m.fullscreen !== "boolean"
                    || typeof m.layout !== "string" || typeof m.title !== "string")
                throw new Error("Invalid dwm monitor at index " + i);
            seen[m.num] = true;
        }
        return message;
    }
    if (message.type === "memory") {
        if (typeof message.percent !== "number" || !isFinite(message.percent)
                || message.percent < 0 || message.percent > 100
                || typeof message.usedGiB !== "number" || !isFinite(message.usedGiB)
                || message.usedGiB < 0)
            throw new Error("Invalid memory sample");
        return message;
    }
    if (message.type === "wallpaper") {
        if (message.path === null) return message;
        if (typeof message.path !== "string" || !/^\/tmp\/dwm-bar-wallpaper-[A-Za-z0-9]+$/.test(message.path)
                || !integer(message.width) || message.width <= 0
                || !integer(message.height) || message.height <= 0)
            throw new Error("Invalid wallpaper sample");
        return message;
    }
    // Ignore future envelope types without interrupting the stream.
    return null;
}

function monitorForScreen(state, screen) {
    if (!state || !screen)
        return null;
    var matches = state.monitors.filter(function(m) {
        return m.x === screen.x && m.y === screen.y
            && m.width === screen.width && m.height === screen.height;
    });
    return matches.length === 1 ? matches[0] : null;
}

function visibleTags(monitor) {
    if (!monitor)
        return [];
    var bits = monitor.selected | monitor.occupied | monitor.urgent;
    var result = [];
    for (var i = 1; i <= tagCount; ++i) {
        if (bits & (1 << (i - 1)))
            result.push(i);
    }
    return result;
}

function selectedIndex(bits) {
    for (var i = 1; i <= tagCount; ++i) {
        if (bits & (1 << (i - 1)))
            return i;
    }
    return 0;
}

function nextTag(bits, direction) {
    var selected = selectedIndex(bits);
    if (!selected)
        return 0;
    return ((selected - 1 + direction) % tagCount + tagCount) % tagCount + 1;
}

function tagLabel(state, index) {
    if (index === 10)
        return "10";
    var label = state && state.tags[index - 1];
    return typeof label === "string" && label.length ? label : String(index);
}
