import QtQuick 2.2
import QtQuick.Controls 2.12

/**
 * @TODO Cannot position text inisde the rectangle using style
 */
Rectangle {
    id: textFieldWrapper

    property alias placeholderText: fakePlaceholderTextComponent.text
    property alias showUnderline: underline.visible
    property alias inputMethodHints: innerTextField.inputMethodHints

    // easiest way to notify about it that I found..
    signal lostActiveFocus

    onFocusChanged: {
        if(focus) {
            innerTextField.focus = true
            innerTextField.forceActiveFocus()
        } else {
            innerTextField.focus = false
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("ITF: clicked")
        }
    }

    width: 208
    height: 44
    color: tertiaryColor

    TextField {

        id: innerTextField
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        width: 204

        // workaround for the bug that sometimes positions cursor vertically a little wrong until first char input
        onFocusChanged: {
            if(focus) {
                var origText = text
                text = origText + " "
                text = origText
            }
        }
        onActiveFocusChanged: {
            if(!activeFocus) {
                lostActiveFocus()
            }
        }

        // To fight against disappearing placeholder
        Text {

            id: fakePlaceholderTextComponent
            anchors.fill: parent
            anchors.leftMargin: 7
            anchors.bottomMargin:6
            font: parent.font
            horizontalAlignment: parent.horizontalAlignment
            verticalAlignment: parent.verticalAlignment
            opacity: !parent.text.length ? 1 : 0
            color: "darkgray"
            clip: contentWidth > width;
            elide: Text.ElideRight
            renderType: Text.NativeRendering
            Behavior on opacity { NumberAnimation { duration: 90 } }
        }

        Image {
            id: xbutton
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: sourceSize.width / 2
            height: sourceSize.height / 2
            source: "qrc:/images/text_edit_x"
            visible: parent.text.length > 0 && parent.activeFocus === true

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    parent.parent.text = ""
                }
            }
        }
    }

    Rectangle {
        id: underline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: tertiaryColor
    }
}
