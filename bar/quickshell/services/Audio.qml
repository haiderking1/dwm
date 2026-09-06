pragma Singleton
import QtQuick
import Quickshell
import Quickshell.Io
import Quickshell.Services.Pipewire

Scope {
    id: root

    readonly property var sink: Pipewire.defaultAudioSink
    readonly property bool available: Pipewire.ready && sink !== null && sink.ready && sink.audio !== null
    readonly property bool muted: available && sink.audio.muted
    readonly property int percent: available ? Math.round(sink.audio.volume * 100) : 0
    property bool hasPavucontrol: false
    readonly property string icon: percent === 0 ? "" : (percent < 50 ? "" : "")
    readonly property string text: !available ? "VOL --" : (muted ? "MUTE" : percent + "% " + icon)

    PwObjectTracker {
        objects: root.sink ? [root.sink] : []
    }

    function toggleMute() {
        if (available)
            sink.audio.muted = !sink.audio.muted;
    }

    function adjust(steps) {
        if (available)
            sink.audio.volume = Math.max(0, Math.min(1, sink.audio.volume + steps * 0.05));
    }

    function openMixer() {
        if (hasPavucontrol)
            Quickshell.execDetached(["env", "GTK_THEME=Adwaita:dark", "pavucontrol"]);
        else
            console.warn("pavucontrol is not installed");
    }

    Process {
        command: ["sh", "-c", "command -v pavucontrol >/dev/null 2>&1"]
        running: true
        onExited: (exitCode, exitStatus) => root.hasPavucontrol = exitCode === 0
    }
}
