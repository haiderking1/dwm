import QtQuick
import Quickshell.Services.SystemTray
import "../theme"

Pill {
    id: root
    required property var panelWindow
    horizontalPadding: Theme.trayPadding
    visible: SystemTray.items.values.length > 0

    Row {
        height: root.contentHeight
        spacing: Theme.traySpacing
        Repeater {
            model: SystemTray.items
            TrayItem {
                required property SystemTrayItem modelData
                item: modelData
                panelWindow: root.panelWindow
                height: root.contentHeight
            }
        }
    }
}
