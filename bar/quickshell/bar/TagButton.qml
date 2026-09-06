import QtQuick
import "../theme"
import "../services"
import "../services/Protocol.js" as Protocol

Item {
    id: root
    required property int tagIndex
    required property var monitor
    readonly property int bit: 1 << (tagIndex - 1)
    readonly property bool selected: !!(monitor.selected & bit)
    readonly property bool urgent: !!(monitor.urgent & bit)

    width: Math.ceil(label.implicitWidth) + 2 * (Theme.tagPadding + Theme.tagMargin)
    height: Theme.pillHeight - 2

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Theme.tagMargin
        anchors.rightMargin: Theme.tagMargin
        radius: Theme.tagRadius
        color: root.urgent ? Theme.red : (pointer.containsMouse ? Theme.hover : "transparent")
        gradient: root.selected && !root.urgent ? selectedGradient : null
        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.InOutQuad } }

        Gradient {
            id: selectedGradient
            GradientStop { position: 0; color: pointer.containsMouse ? Theme.activeHoverTop : Theme.activeTop }
            GradientStop { position: 1; color: pointer.containsMouse ? Theme.activeHoverBottom : Theme.activeBottom }
        }

        BarText {
            id: label
            anchors.centerIn: parent
            text: Protocol.tagLabel(Bridge.state, root.tagIndex)
            color: root.urgent ? Theme.background
                : root.selected ? Theme.activeText
                : pointer.containsMouse ? Theme.text : Theme.inactive
            Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.InOutQuad } }
        }

        MouseArea {
            id: pointer
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            onClicked: mouse => Bridge.command(mouse.modifiers & Qt.ShiftModifier ? "move" : "view", root.monitor, root.tagIndex)
            // Let the enclosing workspace pill handle wheel input.
            onWheel: wheel => wheel.accepted = false
        }
    }
}
