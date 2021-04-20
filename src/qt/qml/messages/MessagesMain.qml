import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

Rectangle {
    anchors.fill: parent
    color:"transparent"

    Header {
        id: header
        color:"transparent"
        height: 40
        anchors.left: parent.left
        anchors.right: parent.right
    }

    SwipeView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom

        clip: true

        currentIndex: header.tabIndex
        onCurrentIndexChanged: header.tabIndex = currentIndex

        Rectangle {
            clip: true
            color:"transparent"

            MessageList {
                anchors.fill: parent
                anchors.topMargin: 5
            }
        }

        Rectangle {
            clip: true
            color:"transparent"

            StatusList {
                anchors.fill: parent
                anchors.topMargin: 5
            }
        }

        Rectangle {
            color: tertiaryColor
        }
    }

    DropShadow {
        anchors.fill: header
        verticalOffset: 2
        radius: 10
        samples: 20
        color: "#80000000"
        source: header
    }
}
