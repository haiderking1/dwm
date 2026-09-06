import QtQuick
import "../theme"

Rectangle {
    id: root
    default property alias content: body.data
    property real horizontalPadding: Theme.pillPadding
    readonly property real contentHeight: height - border.width * 2

    implicitHeight: Theme.pillHeight
    implicitWidth: body.childrenRect.width + 2 * (horizontalPadding + border.width)
    height: implicitHeight
    width: implicitWidth
    radius: Theme.pillRadius
    color: Theme.pill
    border.width: 1
    border.color: Theme.border

    Item {
        id: body
        x: root.horizontalPadding + root.border.width
        y: root.border.width
        width: root.width - 2 * x
        height: root.contentHeight
    }
}
