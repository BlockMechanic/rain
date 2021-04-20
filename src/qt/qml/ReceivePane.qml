import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    color: "transparent"
    id: receivePane

    property string address

    ColumnLayout {
        id: qrColumn
        anchors.fill: parent
            
		Image {
			id: qrImage

			source: "image://qr/rain:" + address

			Layout.alignment: Qt.AlignHCenter
			Layout.preferredWidth: 300
			Layout.preferredHeight: 300

			MouseArea {
				anchors.fill: parent
				onClicked: {
					textAnimation.running = true
					clipboardText.text = qsTr("Address copied to clipboard")
					copyToClipboard(address)
				}
			}
		}
		
		Text {
			id: clipboardText
			//Layout.fillWidth: true
			Layout.alignment: Qt.AlignHCenter
			font.pointSize: 12
			color: textColor
			text: qsTr("Tap image copy to clipboard")
			horizontalAlignment: Text.AlignHCenter

			NumberAnimation {
				id: textAnimation
				target: clipboardText
				property: "opacity"
				from: 0
				to: 1
				duration: 1000
			}
		}
		
		Text {
			id: addressText
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignHCenter
			width: qrImage.width
			font.pointSize: 12
			color: textColor
			text: address
			horizontalAlignment: Text.AlignHCenter
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignHCenter
			Button {
				id: addLabelButton
				Layout.alignment: Qt.AlignLeft
				text: qsTr("Add Label")
				onClicked: {
					//stackView.pop()
				}
				contentItem: Text {
					text: addLabelButton.text
					font: addLabelButton.font
					//opacity: enabled ? 1.0 : 0.3
					color: textColor
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				background: Rectangle {
					implicitWidth: 100
					implicitHeight: 40
					opacity: enabled ? 1 : 0.3
					color:bgColor
					border.color: tertiaryColor
					border.width: 1
					radius: 2
				}
			}
			Button {
				id: generateButton
				text: qsTr("Generate")
				Layout.alignment: Qt.AlignHCenter
				onClicked: {}

				contentItem: Text {
					text: generateButton.text
					font: generateButton.font
					//opacity: enabled ? 1.0 : 0.3
					color: textColor
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}

				background: Rectangle {
					implicitWidth: 100
					implicitHeight: 40
					opacity: enabled ? 1 : 0.3
					color:bgColor
					border.color: tertiaryColor
					border.width: 1
					radius: 2
				}

			}
			Button {
				id:shareButton
				text: qsTr("Share")
				Layout.alignment: Qt.AlignRight
				onClicked: {}

				contentItem: Text {
					text: shareButton.text
					font: shareButton.font
					//opacity: enabled ? 1.0 : 0.3
					color: textColor
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}

				background: Rectangle {
					implicitWidth: 100
					implicitHeight: 40
					opacity: enabled ? 1 : 0.3
					color:bgColor
					border.color: tertiaryColor
					border.width: 1
					radius: 2
				}
			}
		}
    }
}
