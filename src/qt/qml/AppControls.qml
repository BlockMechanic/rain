import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3

Rectangle {
    id: appControls
    color:"transparent"

    signal close()
    signal minimized()
    signal maximized()

    property point startMousePos
    property point startWindowPos
    property size startWindowSize

    function absoluteMousePos(mouseArea) {
        var windowAbs = mouseArea.mapToItem(null, mouseArea.mouseX, mouseArea.mouseY)
        return Qt.point(windowAbs.x + appWindow.x,
                        windowAbs.y + appWindow.y)
    }

    FontLoader { id: awesome; source: "qrc:/fonts/FontAwesome" }

    Item {
        id: titleBar
        anchors.fill: parent

        RowLayout {
            id: pricesWidget
            anchors.right: parent.left

            function updateValue(btcUSD,rainUSD) {
                btcUsdText.text = btcUSD
                usdXlqText.text = rainUSD
            }

            Text {
                id: btcUsdText
                horizontalAlignment: Text.AlignLeft
                font.styleName: "Bold"
                font.pointSize: 12
                Layout.fillWidth: true
                color: textColor
            }

            Text {
                id: usdXlqText
                horizontalAlignment: Text.AlignRight
                font.styleName: "Bold"
                font.pointSize: 12
                Layout.fillWidth: true
                color: textColor
            }
        }

        Item {
            id:titleBarToolsMinimize
            height: parent.height
            width: parent.height
            anchors.right: titleBarToolsMaximize.left

            Text {
                id: minimizeBtn
                font.pixelSize: 18
                font.family: awesome.name
                color: textColor
                text: "\uf068"
                anchors.centerIn: parent
            }

            MouseArea {
                id: minimizeRegion
                cursorShape: Qt.PointingHandCursor
                anchors.fill: minimizeBtn
                onClicked: appControls.minimized()
            }
        }

        Item {
            id:titleBarToolsMaximize
            height: parent.height
            width: 14
            anchors.right: titleBarToolsClose.left

            Text{
                id: maximizeBtn
                font.pixelSize: 18
                font.family: awesome.name
                color: textColor
                text: "\uf065"
                anchors.centerIn: parent
            }

            MouseArea{
                id: maximizeRegion
                cursorShape: Qt.PointingHandCursor
                anchors.fill: maximizeBtn
                onClicked: appControls.maximized()
            }
        }

        Item {
            id:titleBarToolsClose
            height: parent.height
            width: parent.height
            anchors.right: parent.right

            Text {
                id: closeBtn
                font.pixelSize: 18
                font.family: awesome.name
                color: textColor
                text: "\uf00d"
                anchors.centerIn: parent
            }

            MouseArea {
                id:closeRegion
                cursorShape: Qt.PointingHandCursor
                anchors.fill: closeBtn
                onClicked: appControls.close()
            }
        }

        MouseArea {
            id: titleRegion
            //anchors.fill: parent;
            cursorShape: Qt.DragMoveCursor
            property variant clickPos: "1,1"

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            width: titleBar.width - titleBarToolsClose.width - titleBarToolsMaximize.width - titleBarToolsMinimize.width

            onPressed: {
                startMousePos = absoluteMousePos(titleRegion)
                startWindowPos = Qt.point(appWindow.x, appWindow.y)
                startWindowSize = Qt.size(appWindow.width, appWindow.height)
            }
            onMouseYChanged: {
                var abs = absoluteMousePos(titleRegion)
                var newWidth = Math.max(appWindow.minimumWidth, startWindowSize.width - (abs.x - startMousePos.x))
                var newX = startWindowPos.x - (newWidth - startWindowSize.width)
                appWindow.x = newX
                appWindow.width = newWidth
            }
        }
    }
}
