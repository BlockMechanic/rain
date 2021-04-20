import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    color: "transparent"

    TabBar {
        id: mntabBar
        width: parent.width
        background: Rectangle {
            color: bgColor
        }
        TabButton {
            contentItem: Text {
                text: qsTr("Network")
                opacity: mntabBar.currentIndex == 0 ? 1.0 : 0.3
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: "transparent"
            }
        }

        TabButton {
            contentItem: Text {
                text: qsTr("Mine")
                opacity: mntabBar.currentIndex == 1 ? 1.0 : 0.3
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: "transparent"
            }
        }
    }

    SwipeView {
        id: mnswipeview
        anchors.topMargin: mntabBar.height
        currentIndex: mntabBar.currentIndex
        anchors.fill: parent
        Rectangle {
            color: "transparent"

            ListView {
                id: masternodeView
                anchors.fill: parent
                anchors.margins: 10
                currentIndex: -1

                model: mnModel

                Row {
                    anchors.fill: parent
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Address")
                        color: textColor
                        width: parent.width / 4
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Protocol")
                        color: textColor
                        width: parent.width / 4
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Status")
                        color: textColor
                        width: parent.width / 4
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Active")
                        color: textColor
                        width: parent.width / 4
                    }
                }

    delegate: Rectangle {
        implicitWidth: 100
        implicitHeight: 50
        Text {
            text: display
        }
    }

            }
        }
        Rectangle {
            color: "transparent"
            ListView {
                id: mymasternodeView
                anchors.margins: 10
                anchors.fill: parent
                currentIndex: -1

                model: mnModel
                Row {
                    anchors.fill: parent
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Alias")
                        color: textColor
                        width: parent.width / 6
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("IP")
                        color: textColor
                        width: parent.width / 6
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Port")
                        color: textColor
                        width: parent.width / 6
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Privkey")
                        color: textColor
                        width: parent.width / 6
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Txid")
                        color: textColor
                        width: parent.width / 6
                    }
                    Text {
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("index")
                        color: textColor
                        width: parent.width / 6
                    }
                }

    delegate: Rectangle {
        implicitWidth: 100
        implicitHeight: 50
        Text {
            text: display
        }
    }
            }
        }
    }
    ToolBar {
        background: Rectangle {
            anchors.fill: parent
            color: bgColor
        }
        anchors.bottom: parent.bottom

        height: 40
        width: parent.width
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "&Create Masternode"
                contentItem: Text {
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: bgColor
                }
                //                onClicked: {
                //                    confirmationDialog.open()
                //                }
                font: theme.thinFont
            }
            ToolButton {
                text: "&Start ALL"
                contentItem: Text {
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: bgColor
                }
                //                onClicked: {
                //                    confirmationDialog.open()
                //                }
                font: theme.thinFont
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/

