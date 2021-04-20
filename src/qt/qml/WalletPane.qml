import QtQuick 2.12
import QtQml 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

SwipeView {
    id: assetsSwipeview
    objectName: "assetsSwipeview"

    FrontPane {
        id: frontPage
        objectName: "frontPage"
    }

    AssetPane {
        id: assetPage
        objectName: "assetPage"
    }

    Dialog {
        id: transactionInfoDialog

        property string transactionInfoString

            background: Rectangle {
                anchors.fill: parent
                color : bgColor
            }

        modal: true
        focus: true
        x: (parent.width - width) / 2
        y: parent.height / 15
        width: Math.min(parent.width, parent.height) / 8 * 7
        contentHeight: transactionInfoText.height

        Text {
            id: transactionInfoText
            width: transactionInfoDialog.availableWidth
            text: transactionInfoDialog.transactionInfoString.slice(0, -18) // Get rid of the last newline
            wrapMode: Text.Wrap
            font: theme.thinFont
            color: textColor
        }
    }
}

