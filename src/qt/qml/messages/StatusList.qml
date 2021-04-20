import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

Rectangle {
    id: statusPage
    color:"transparent"

	RoundButton {
		id: otherBtn
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

	  ColumnLayout {
        anchors.fill: parent
        ListView {
            id: myStatus
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListElement {
                id: myStatusLayout
            }
        }

        ListModel {
            id: fruitModel

        }
        ListView {
            id: otherStatus
            model: fruitModel
            delegate: Row {
                Text { text: "Fruit: " + name }
                Text { text: "Cost: $" + cost }
            }
        }
    }
}
