import QtQuick
import QtQuick.Effects
import Quickshell
import Quickshell.Services.SystemTray
import "../theme"

Item {
    id: root
    required property SystemTrayItem item
    required property var panelWindow
    width: Theme.iconSize

    function openMenu() {
        if (!item.hasMenu)
            return;
        const position = panelWindow.itemPosition(root);
        // Installed 0.3.1 takes a window and WINDOW-RELATIVE coordinates.
        item.display(panelWindow, Math.round(position.x), Math.round(position.y + height));
    }

    Image {
        id: icon
        anchors.centerIn: parent
        width: Theme.iconSize
        height: Theme.iconSize
        sourceSize.width: Theme.iconSize
        sourceSize.height: Theme.iconSize
        source: root.item.icon
        fillMode: Image.PreserveAspectFit
        smooth: true
        opacity: root.item.status === Status.Passive ? 0.5 : 1
        layer.enabled: root.item.status === Status.NeedsAttention
        layer.effect: MultiEffect { brightness: 0.15 }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.MiddleButton)
                root.item.secondaryActivate();
            else if (mouse.button === Qt.RightButton || root.item.onlyMenu)
                root.openMenu();
            else
                root.item.activate();
        }
        onWheel: wheel => {
            // SNI expects the raw angle delta, not a normalized +/-1 step.
            if (wheel.angleDelta.y)
                root.item.scroll(wheel.angleDelta.y, false);
            if (wheel.angleDelta.x)
                root.item.scroll(wheel.angleDelta.x, true);
            wheel.accepted = true;
        }
    }
}
