import QtQuick 2.12

Rectangle {
    id: wholeAddPhoneLine
    height: 42
    signal clicked
    
    color:bgColor

    Item {
        id: addPhoneRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        

        Image {
            id: phonePlus
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/images/green-plus"
            width: 20
            height: 20
        }

        Text {
            id: addPhoneButton
            anchors.left: phonePlus.right
            anchors.verticalCenter: phonePlus.verticalCenter
            anchors.leftMargin: 10
            anchors.right: parent.right
            text: "add phone"
            color: textColor
        }

        Rectangle {
            id: underline
            anchors.top: parent.bottom
            anchors.left: addPhoneButton.left
            anchors.right: addPhoneButton.right

            anchors.topMargin: -1
            height: 1
            color: bgColor
        }

        MouseArea {
            id: addPhoneArea
            anchors.fill: parent
            onClicked: {
                wholeAddPhoneLine.clicked()
            }
        }
    }

}
