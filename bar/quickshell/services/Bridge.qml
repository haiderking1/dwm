pragma Singleton
import QtQuick
import Quickshell
import Quickshell.Io
import "Protocol.js" as Protocol

Scope {
    id: root

    readonly property string executable: "/home/soka/.local/bin/dwm-bar-bridge"
    property var state: null
    property var memory: null
    property var wallpaper: null
    readonly property bool connected: state !== null

    function acceptLine(line) {
        if (!line.trim())
            return;
        try {
            const message = Protocol.decode(line);
            if (!message)
                return;
            if (message.type === "state")
                state = message.data;
            else if (message.type === "memory")
                memory = { percent: message.percent, usedGiB: message.usedGiB };
            else if (message.type === "wallpaper")
                wallpaper = message.path ? message : null;
        } catch (error) {
            console.warn("dwm bridge: ignoring invalid JSON/state: " + error.message);
        }
    }

    function disconnect() {
        state = null;
        memory = null;
        wallpaper = null;
    }

    function monitorForScreen(screen) {
        return Protocol.monitorForScreen(state, screen);
    }

    function command(action, monitor, tag) {
        if (!connected || !monitor || (action !== "view" && action !== "move")
                || !Protocol.integer(tag) || tag < 1 || tag > Protocol.tagCount)
            return;
        Quickshell.execDetached([executable, action, String(monitor.num), String(tag)]);
    }

    function cycle(monitor, direction) {
        if (!monitor)
            return;
        const tag = Protocol.nextTag(monitor.selected, direction);
        if (tag)
            command("view", monitor, tag);
    }

    Process {
        id: watcher
        command: [root.executable, "--watch"]
        running: true
        stdout: SplitParser {
            splitMarker: "\n"
            onRead: data => root.acceptLine(data)
        }
        stderr: SplitParser {
            splitMarker: "\n"
            onRead: data => { if (data.trim()) console.warn("dwm bridge: " + data); }
        }
        onRunningChanged: {
            if (!running)
                root.disconnect();
        }
        onExited: (exitCode, exitStatus) => {
            root.disconnect();
            console.warn("dwm bridge stopped, exit " + exitCode + "; retrying in at most 2s");
        }
    }

    // A repeating timer also handles FailedToStart, which need not emit exited.
    Timer {
        interval: 2000
        repeat: true
        running: true
        onTriggered: {
            if (!watcher.running)
                watcher.running = true;
        }
    }
}
