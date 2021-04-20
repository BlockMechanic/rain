import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

Controls.TabButton {
    id: tab
    height: 40
    anchors.bottom: parent.bottom

    contentItem: Text {
        font.pixelSize: 15
        font.weight: Font.ExtraBold
        color: tab.checked ? textColor : "#777777"
        text: tab.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
    }

    background: Rectangle {
        color: tertiaryColor
        width: parent.width
        height: 3
        visible: tab.checked
        anchors.bottom: parent.bottom
    }
}
