import QtQuick
import Quickshell
import "../services"

// X11 needs a compositor for alpha. Sample the actual root wallpaper instead;
// this keeps the same appearance and follows Waypaper without another daemon.
Item {
    required property ShellScreen screen
    anchors.fill: parent
    clip: true
    Image {
        x: -parent.screen.x
        y: -parent.screen.y
        width: Bridge.wallpaper ? Bridge.wallpaper.width : 0
        height: Bridge.wallpaper ? Bridge.wallpaper.height : 0
        source: Bridge.wallpaper ? "file://" + Bridge.wallpaper.path : ""
        cache: false
        fillMode: Image.Stretch
    }
}
