import QtQuick 2.12

Rectangle {
    id: wholeEditor
    property alias type: phoneTypeButton.text

    //  user clicked delete button
    signal selfDeletionWanted

    height: 42

    onFocusChanged: {
        phoneNumberField.focus = focus
    }
    color: bgColor

    Rectangle {
        id: deleteButtonBlock
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 90
        color: bgColor

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 24

            font.pixelSize: 18
            color: "red"
            text: "Delete"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                selfDeletionWanted()
            }
        }
    }
    Rectangle {
        id: swipablePart
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width
        color:bgColor

        Behavior on anchors.leftMargin {
            NumberAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }

        Image {
            id: deleteIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            width: 24
            height: 26

            source: "qrc:/images/red_minus"

            MouseArea {
                id: deletionIconArea
                anchors.fill: parent
                onClicked: wholeEditor.state = "deletionQuery"
            }
        }

        Rectangle {
            id: typeAndNumberBlock
            anchors.top: parent.top
            anchors.left: deleteIcon.right
            anchors.leftMargin: 10
            anchors.bottom: parent.bottom
            width: parent.width - deleteIcon.width - 10
            color: bgColor

            Rectangle {
                id: buttonAndChevronBlock
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: childrenRect.width
                color:bgColor

                Text {
                    id: phoneTypeButton
                    text: "home"
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 75
                    clip: true
                    elide: Text.ElideRight
                    color: textColor
                }

                Image {
                    id: chevronIcon
                    anchors.left: phoneTypeButton.right
                    anchors.verticalCenter: phoneTypeButton.verticalCenter
                    anchors.leftMargin: 5
                    width: 8
                    height: 12

                    source: "qrc:/images/chevron"

                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        function selectionCompletionHandler(selectedText) {
                            phoneTypeButton.text = selectedText
                            pageStack.currentItem.selectionCompleted.disconnect(selectionCompletionHandler)
                        }

                        pageStack.push("qrc:/qml/ListSelectorPage.qml",
                                       {
                                           leftButtonText: "Cancel",
                                           title: "Label",
                                           items: ["home", "work", "mobile", "company main", "work fax",
                                                   "home fax", "assistant", "pager", "car", "radio"],
                                           selectedText: phoneTypeButton.text,
                                       })
                        pageStack.currentItem.selectionCompleted.connect(selectionCompletionHandler)

                    }
                }
            }

            // @TODO add some proper gradient
            Rectangle {
                id: verticalSeparator
                anchors.left: buttonAndChevronBlock.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: 4
                anchors.leftMargin: 4

                width: 1
                color: bgColor
            }

            IOSTextField {
                id: phoneNumberField
                anchors.left: verticalSeparator.right
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 24
                color:bgColor

                placeholderText: "Phone"
                showUnderline: false
                inputMethodHints: Qt.ImhDialableCharactersOnly
                onLostActiveFocus: {
                    wholeEditor.state = ""
                }
            }

            Rectangle {
                id: underline
                anchors.top: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: -1

                height: 1
                color: bgColor
            }

        }
    }

    states: [
        State {
            name: "deletionQuery"
            PropertyChanges {
                target: swipablePart
                anchors.leftMargin: -deleteButtonBlock.width
            }
        }
    ]


}
