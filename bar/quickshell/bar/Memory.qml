import QtQuick
import "../services"

Pill {
    BarText {
        height: parent.height
        text: Bridge.memory
            ? "RAM " + Math.round(Bridge.memory.percent) + "% (" + Bridge.memory.usedGiB.toFixed(1) + "G)"
            : "RAM --% (--G)"
    }
}
