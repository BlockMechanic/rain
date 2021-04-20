import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    id: consolePane
    color:"transparent"

    signal executeCommand(string commandText)

    function showReply(msg) {
        consoleText.text = msg
        consoleFlickable.contentY = consoleFlickable.contentHeight - consoleFlickable.height
    }

    ColumnLayout {
        id: consoleColumn
        anchors.fill: parent

        Flickable {
            id: consoleFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: consoleText.width
            contentHeight: consoleText.height
            clip: true

            TextArea {
                id: consoleText
                width: parent.parent.width
                text: rainTr("RPCConsole", "WARNING: Do not use this console without fully understanding the ramifications of a command.")
                font.pointSize: 10
                color: textColor
                wrapMode: Text.Wrap
                readOnly: true
                selectByMouse: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: ">"
                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 12
                font.bold: true
                color: textColor
            }

            TextField {
                id: consoleInput
                Layout.fillWidth: true

                focus: true

                font.family: robotoThin.name
                font.styleName: "Thin"
                font.pointSize: 12
                font.bold: true
                color: textColor
                
                background: Rectangle {
                    color: "transparent"
                    radius: 2
                    border.color: tertiaryColor
                    border.width: consoleInput.visualFocus ? 1 : 0
                }
                
                onAccepted: {
                    executeCommand(text);
                    text = ""
                }
            }

            Button {
                id: runcmdbtn
                text: qsTr("Run")

                contentItem: Text {
                    text: runcmdbtn.text
                    font: runcmdbtn.font
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
				onClicked: {
                    executeCommand(consoleInput.text);
                    consoleInput.text = ""
				}
                background: Rectangle {
                    color: bgColor
                    radius: 2
                }
            }
        }
    }
}
