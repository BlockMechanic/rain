import QtQuick 2.12
import QtQuick.Controls 2.12

ListView {
    model: MessagesModel{}

    delegate: MessageListItem {
    }
	RoundButton {
		id: newchatBtn
		radius: 90
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.rightMargin: 10
		anchors.bottomMargin: 10
		text: "+"
		onClicked: {
		  //active = true
		}
		background: Rectangle {
		    radius:parent.width
			color: tertiaryColor
		}
	}    
}
