import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

Drawer {
    id: drawer

    //
    // Default size options
    //
    implicitHeight: parent.height
    implicitWidth: Math.min (parent.width > parent.height ? 320 : 280,
                             Math.min (parent.width, parent.height) * 0.90)

    //
    // Icon properties
    //
    
    property string iconTitle: ""
    property string iconSource: ""
    property string iconSubtitle: ""
    property size iconSize: Qt.size (48, 48)

    Rectangle {
        anchors.fill: parent
        color:bgColor
    }

    //
    // List model that generates the page selector
    // Options for selector items are:
    //     - spacer: acts an expanding spacer between to items
    //     - pageTitle: the text to display
    //     - separator: if the element shall be a separator item
    //     - separatorText: optional text for the separator item
    //     - pageIcon: the source of the image to display next to the title
    //
    property alias items: listView.model
    property alias index: listView.currentIndex

    //
    // Execute appropiate action when the index changes
    //
    onIndexChanged: {
        var isSpacer = false
        var isSeparator = false
        var item = items.get (index)

        if (typeof (item) !== "undefined") {
            if (typeof (item.spacer) !== "undefined")
                isSpacer = item.spacer

            if (typeof (item.separator) !== "undefined")
                isSpacer = item.separator

            if (!isSpacer && !isSeparator)
                actions [index]()
        }
    }

    //
    // A list with functions that correspond with the index of each drawer item
    // provided with the \a pages property
    //
    // For a string-based example, check this SO answer:
    //     https://stackoverflow.com/a/26731377
    //
    // The only difference is that we are working with the index of each element
    // in the list view, for example, if you want to define the function to call
    // when the first item of the drawer is clicked, you should write:
    //
    //     actions: {
    //         0: function() {
    //             console.log ("First item clicked!")
    //         },
    //
    //         1: function() {}...,
    //         2: function() {}...,
    //         n: function() {}...
    //     }
    //
    property var actions

    //
    // Main layout of the drawer
    //
    ColumnLayout {
        spacing: 0
        anchors.margins: 0
        anchors.fill: parent

        //
        // Icon controls
        //
        Rectangle {
            z: 1
            height: 120
            id: iconRect
            Layout.fillWidth: true
            color:bgColor

            ColumnLayout {
                anchors.fill: parent
                spacing: 4
                Layout.fillWidth: true
                Layout.fillHeight: true

                Image {
                  source: "qrc:/icons/rain"
                  sourceSize: Qt.size (48, 48)
                  Layout.alignment: Qt.AlignHCenter
                }                

                Label {
                    color: "#fff"
                    text: iconTitle
                    font.weight: Font.Medium
                    font.pixelSize: 16
                    Layout.fillWidth: true
                    //Layout.topMargin: 4
                    horizontalAlignment: Text.AlignHCenter
                }
            }          
        }

        //
        // Page selector
        //

		ListView {
			z: 0
			id: listView
			currentIndex: -1
			Layout.fillWidth: true
			Layout.fillHeight: true
			Component.onCompleted: currentIndex = 0

			delegate: DrawerItem {
				model: items
				width: parent.width
				pageSelector: listView

				onClicked: {
					if (listView.currentIndex !== index)
						listView.currentIndex = index
					drawer.close()
				}
			}
			//ScrollIndicator.vertical: ScrollIndicator { }
		} 
    }
}
