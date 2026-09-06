import QtQuick
import "../theme"

Text {
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSize
    color: Theme.text
    textFormat: Text.PlainText
    renderType: Text.NativeRendering
    verticalAlignment: Text.AlignVCenter
}
