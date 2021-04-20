import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Rectangle {
    id: item
    height: 90
    width: parent.width
    color:"transparent"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15

        ProfileImage {
            source: model.image
        }

        RowLayout {
            Layout.leftMargin: 10

            ColumnLayout {
                Layout.fillWidth: true

                Text {
                    text: model.name
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: textColor
                }

                Text {
                    text: model.lastMessage
                    font.pixelSize: 16
                    color: textColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignRight

                Text {
                    text: model.lastActivity
                    font.pixelSize: 14
                    color: tertiaryColor
                }

                Rectangle {
                    color: tertiaryColor
                    width: 24
                    height: 24
                    radius: width / 2
                    Layout.alignment: Qt.AlignHCenter
                    opacity: model.newMessages > 0 ? 1 : 0

                    Text {
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        text: model.newMessages
                        color: tertiaryColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 90
        anchors.rightMargin: 15
        height: 1
        color: index + 1 == count ? "transparent" : tertiaryColor
    }
}
