import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Dialog {
    title: ""
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true
    focus: true
	background: Rectangle {
		anchors.fill: parent
		color : bgColor
	}
    property string mode: ""
    anchors.centerIn: parent
    width: Math.round(Math.min(parent.width, parent.height) / 3 * 2)

    contentItem: Rectangle {
        anchors.fill: parent
        color: bgColor
		ColumnLayout {
		    anchors.fill: parent
		    spacing : 5
		
			TextField {
				id: passphrase
				placeholderText: qsTr("Enter Passphrase")
				placeholderTextColor: textColor
				horizontalAlignment: TextInput.AlignHCenter
				color:textColor
				Layout.fillWidth: true
				height: 20

				background: Rectangle {
					width:  parent.width
					height: parent.height
					color: "transparent"
					border.color: passphrase.enabled ? tertiaryColor : "transparent"
				}
				echoMode: TextInput.PasswordEchoOnEdit
				//passwordCharacter:
			}

			TextField {
				id: newpassphrase
				placeholderText: qsTr("Enter New Passphrase")
				placeholderTextColor: textColor
				horizontalAlignment: TextInput.AlignHCenter
				color:textColor
				Layout.fillWidth: true
				height: 20

				background: Rectangle {
					width:  parent.width
					height: parent.height
					color: "transparent"
					border.color: newpassphrase.enabled ? tertiaryColor : "transparent"
				}
				echoMode: TextInput.PasswordEchoOnEdit
				//passwordCharacter:
			}

			TextField {
				id: repeatpassphrase
				placeholderText: qsTr("Repeat Passphrase")
				placeholderTextColor: textColor
				horizontalAlignment: TextInput.AlignHCenter
				color:textColor
				Layout.fillWidth: true
				height: 20

				background: Rectangle {
					width:  parent.width
					height: parent.height
					color: "transparent"
					border.color: repeatpassphrase.text === passphrase.text ? tertiaryColor :"red"
				}
				echoMode: TextInput.PasswordEchoOnEdit
				//passwordCharacter:
			}

			CheckBox {
				id: stakingonly
				text: qsTr("Staking Only")
				checked: true
				visible:true

				indicator: Rectangle {
					implicitWidth: 18
					implicitHeight: 18
					x: stakingonly.leftPadding
					y: parent.height / 2 - height / 2
					radius: 3
					border.color: tertiaryColor
					color: "transparent"

					Rectangle {
						width: 12
						height: 12
						x: 3
						y: 3
						radius: 2
						color: tertiaryColor
						visible: stakingonly.checked
					}
				}

				contentItem: Text {
					text: stakingonly.text
					font: stakingonly.font
					opacity: enabled ? 1.0 : 0.3
					color: textColor
					verticalAlignment: Text.AlignVCenter
					leftPadding: stakingonly.indicator.width + stakingonly.spacing
				}
			}
		}
    }

    Component.onCompleted: {
        if(mode==="Encrypt"){
            passphrase.visible=false
            stakingonly.visible=false
        }

        if(mode==="Unlock"){
            newpassphrase.visible=false
            repeatpassphrase.visible=false
        }

        if(mode==="ChangePass"){
            stakingonly.visible=false
        }
//
//        if(mode==="Decrypt"){
//            newpassphrase.visible=false
//            repeatpassphrase.visible=false
//            stakingonly.visible=false
//        }
    }
    
	onAccepted: {

		if(mode==="Encrypt"){
			if(askpassphrasedialognewpassphrase.text === repeatpassphrase.text && newpassphrase.text.length > 8)
				appWindow.encrypt(newpassphrase.text)
		}

		if(mode==="Unlock"){
			if(newpassphrase.text !== "")
				appWindow.unlockWallet(passphrase.text, stakingonly.checked)
		}
	}
	onRejected: askpassphrasedialog.close()
}
