import QtQuick
import "../theme"
import "../services"
import "../services/Protocol.js" as Protocol

Pill {
    id: root
    required property var monitor
    horizontalPadding: Theme.workspacePadding

    MouseArea {
        width: tags.width
        height: root.contentHeight
        acceptedButtons: Qt.NoButton
        onWheel: wheel => {
            const steps = wheelSteps.take(wheel.angleDelta.y);
            if (steps && root.monitor) {
                const tag = Protocol.nextTag(root.monitor.selected, steps);
                if (tag)
                    Bridge.command("view", root.monitor, tag);
            }
            wheel.accepted = true;
        }

        WheelAccumulator { id: wheelSteps }

        Row {
            id: tags
            height: parent.height
            Repeater {
                model: Protocol.visibleTags(root.monitor)
                TagButton {
                    required property int modelData
                    tagIndex: modelData
                    monitor: root.monitor
                }
            }
            BarText {
                visible: !root.monitor
                text: "WS --"
                height: parent.height
            }
        }
    }
}
