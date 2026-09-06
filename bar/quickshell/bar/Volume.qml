import QtQuick
import "../theme"
import "../services"

Pill {
    id: root

    BarText {
        id: label
        height: parent.height
        text: Audio.text
        color: Audio.muted ? Theme.red : Theme.cyan
        Behavior on color { ColorAnimation { duration: 250; easing.type: Easing.InOutQuad } }
    }

    MouseArea {
        x: -root.horizontalPadding - root.border.width
        y: -root.border.width
        width: root.width
        height: root.height
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                Audio.toggleMute();
            else
                Audio.openMixer();
        }
        onWheel: wheel => {
            const steps = wheelSteps.take(wheel.angleDelta.y);
            if (steps)
                Audio.adjust(steps);
            wheel.accepted = true;
        }
        WheelAccumulator { id: wheelSteps }
    }
    // Input bounds must not contribute to the content's measured width.
    implicitWidth: Math.ceil(label.implicitWidth) + 2 * (horizontalPadding + border.width)
}
