import QtQuick 2.12

Rectangle {
    property int tabIndex: 0

    MessagesTabBar {
        id: tabBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 15
        anchors.rightMargin: 15

        currentIndex: tabIndex

        onCurrentIndexChanged: tabIndex = currentIndex
    }
}
