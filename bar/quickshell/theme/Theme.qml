pragma Singleton
import QtQuick

QtObject {
    readonly property int barHeight: 38
    readonly property int pillHeight: 28
    readonly property int verticalMargin: 5
    readonly property int outerMargin: 8
    // Waybar spacing 6 plus each neighboring module's 4px margin.
    readonly property int moduleGap: 14
    readonly property int pillPadding: 14
    readonly property int workspacePadding: 6
    readonly property int trayPadding: 12
    readonly property int pillRadius: 10
    readonly property int tagRadius: 8
    readonly property int tagPadding: 10
    readonly property int tagMargin: 2
    readonly property int iconSize: 18
    readonly property int traySpacing: 8
    readonly property string fontFamily: "JetBrainsMono Nerd Font"
    readonly property int fontSize: 15
    readonly property color background: "#151515"
    readonly property color pill: "#1b1b1f"
    readonly property color border: "#2a2a31"
    readonly property color hover: "#24243a"
    readonly property color text: "#dedede"
    readonly property color inactive: "#8a8a8a"
    readonly property color activeText: "#eeeeee"
    readonly property color cyan: "#19b2a7"
    readonly property color red: "#e17373"
    readonly property color yellow: "#ffb97b"
    readonly property color activeTop: "#5a63a8"
    readonly property color activeBottom: "#3c4280"
    readonly property color activeHoverTop: "#646daa"
    readonly property color activeHoverBottom: "#464d91"
}
