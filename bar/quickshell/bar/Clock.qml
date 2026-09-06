import QtQuick
import Quickshell
import "../theme"

Pill {
    SystemClock { id: clock; precision: SystemClock.Minutes }
    BarText {
        height: parent.height
        text: Qt.formatDateTime(clock.date, "HH:mm")
        color: Theme.yellow
        font.bold: true
    }
}
