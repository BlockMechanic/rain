import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    color:"transparent"

    function updateBalance(formattedBalance,formattedImmature,formattedPending,formattedStake) {
        balanceText.text = formattedBalance
        immatureText.text = formattedImmature
        pendingText.text = formattedPending
        stakeText.text = formattedStake
    }

    function updateWalletValue(formattedValue) {
        valueText.text = formattedValue
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 5
        Text {
            id: balanceText
            horizontalAlignment: Text.AlignHCenter
            font.family: robotoThin.name
            font.styleName: "Bold"
            font.pointSize: 25
            color: textColor
            Layout.fillWidth: true
        }

		Text {
			id: immatureText
			horizontalAlignment: Text.AlignHCenter
			font.family: robotoThin.name
			font.styleName: "Bold"
			font.pointSize: 10
			color: textColor
			Layout.fillWidth: true
			visible : false
		}

		Text {
			id: pendingText
			horizontalAlignment: Text.AlignHCenter
			font.family: robotoThin.name
			font.styleName: "Bold"
			font.pointSize: 10
			color: textColor
			Layout.fillWidth: true
			visible : false
		}

		Text {
			id: stakeText
			horizontalAlignment: Text.AlignHCenter
			font.family: robotoThin.name
			font.styleName: "Bold"
			font.pointSize: 10
			color: textColor
			Layout.fillWidth: true
			visible : false
		}

        Text {
            id: valueText
            objectName: "valueText"

            horizontalAlignment: Text.AlignHCenter
            font.family: robotoThin.name
            font.styleName: "Bold"
            font.pointSize: 12
            color: textColor
            Layout.fillWidth: true
        }
        Text {
          id: emptytransactionListViewText
          text: qsTr("No Transactions")
          Layout.alignment : Qt.AlignHCenter
          font.pixelSize: 40
          font.weight: Font.Light
          color: textColor
          visible: transactionListView.count > 0 ? false : true
        }

        ListView {
            id: transactionListView
            objectName: "transactionListView"

            model: transactionsModel

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment : Qt.AlignHCenter
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            Layout.leftMargin: parent.width/10
            Layout.rightMargin: parent.width/10
            clip: true
            snapMode: ListView.SnapToItem

            delegate : ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 5
                width: transactionListView.width
                RowLayout {
                    id:txitem
                    Layout.fillWidth: true
                    ColumnLayout {
                        Column {
							Text {
								text:  model.label === "" ? model.address.substring(0, 16) + "..." : model.label
								color: textColor
								font.pointSize: 14
								Layout.fillWidth: true
							}
							Text {
								text: model.date.toLocaleString(Qt.locale(), "dd/MM/yyyy hh:mm")
								font.pointSize: 8
								color: textColor
								Layout.fillWidth: true
							}
                        }
                    }

                    Text {
                        text: "<b>" + ((parseFloat(model.amount) > 0) ? "+" : "") + model.amount + "</b>"
                        color: (parseFloat(model.amount) > 0) ? tertiaryColor : "red"
                        horizontalAlignment: Text.AlignRight
                        Layout.fillWidth: true
                    }
                    MouseArea {
                        width:  parent.width
                        height: parent.height
                        onClicked: {
                            transactionInfoDialog.transactionInfoString = model.longdescription
                            transactionInfoDialog.open()
                        }
                        onPressAndHold: copyToClipboard(model.txhash)
                    }
                }
                Rectangle {
                    id: underliner
                    width:  transactionListView.width
                    height: 1
                    color: tertiaryColor
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }
}
