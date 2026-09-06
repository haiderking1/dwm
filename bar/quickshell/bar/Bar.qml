import QtQuick
import Quickshell
import Quickshell.X11
import "../theme"
import "../services"

XPanelWindow {
    id: panel
    readonly property var monitor: Bridge.monitorForScreen(screen)

    anchors { top: true; left: true; right: true }
    implicitHeight: Theme.barHeight
    color: "transparent"
    exclusiveZone: Theme.barHeight
    focusable: false
    aboveWindows: !monitor || !monitor.fullscreen

    Wallpaper { screen: panel.screen }

    Workspaces {
        anchors.left: parent.left
        anchors.leftMargin: Theme.outerMargin
        y: Theme.verticalMargin
        monitor: panel.monitor
    }

    Clock {
        anchors.horizontalCenter: parent.horizontalCenter
        y: Theme.verticalMargin
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.outerMargin
        y: Theme.verticalMargin
        spacing: Theme.moduleGap
        Memory {}
        Volume {}
        Tray { panelWindow: panel }
    }
}
