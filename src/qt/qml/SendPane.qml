import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    color: "transparent"
    id: sendPane

    property int confirmations: 2
    property string confirmationsString: confirmationTargets[confirmations.toString()]

    signal send(string address, string amount, string assetname)
    signal lockunlock(string hash, int index)
    signal changesendbalance(string asset)
    signal setcoincontroloutput(string hash, int index, bool set)
    signal setrbf(bool set)
    signal setcoincontroltarget(int num)

    function setsendbalance(balance){
        sendpagebalance.text=balance
    }

    ColumnLayout {
        id: sendColumn
        spacing: 10

        anchors.fill: parent
        anchors.margins: 10               

        ComboBox {
            id: contact
            model: addressTablemodel
            Layout.fillWidth: true
            editable: true
            currentIndex: -1
            onActivated: {
                addressText.text=contact.currentText
            }

            delegate: ItemDelegate {
                width: contact.width
                contentItem: Text {
                    text: model.label === "" ? model.address : model.label
                    color: textColor
                    font: contact.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    width:parent.width/4
                }
                highlighted: contact.highlightedIndex === index
            }

            indicator: Canvas {
                id: canvass
                x: contact.width - width - contact.rightPadding
                y: contact.topPadding + (contact.availableHeight - height) / 2
                width: 12
                height: 8
                contextType: "2d"

                Connections {
                    target: contact
                    onPressedChanged: canvass.requestPaint()
                }

                onPaint: {
                    context.reset();
                    context.moveTo(0, 0);
                    context.lineTo(width, 0);
                    context.lineTo(width / 2, height);
                    context.closePath();
                    context.fillStyle = textColor;
                    context.fill();
                }
            }

		    contentItem: TextField {
		        id: addressText
		        placeholderText: qsTr("Enter Address...")
		        placeholderTextColor: textColor
		        horizontalAlignment: TextInput.AlignHCenter
		        color:textColor

		        background: Rectangle {
		            width: parent.width
		            height: parent.height
		            color: "transparent"
		            border.color: addressText.visualFocus ? tertiaryColor : textColor
		        }
		    }

            background: Rectangle {
                color:"transparent"
                border.color: contact.pressed ? tertiaryColor : textColor
                border.width: contact.visualFocus ? 2 : 1
                radius: 2
                implicitHeight: 30
            }

            popup: Popup {
                y: contact.height - 1
                width: contact.width
                implicitHeight: contentItem.implicitHeight
                padding: 1

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: contact.popup.visible ? contact.delegateModel : null
                    currentIndex: contact.highlightedIndex

                    ScrollIndicator.vertical: ScrollIndicator { }
                }

                background: Rectangle {
                    color:"transparent"
                    border.color: tertiaryColor
                    radius: 2
                }
            }
        }

        TextField {
            id: amountField
            Layout.fillWidth: true
            placeholderText: "0.0000"
            placeholderTextColor: textColor
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: 0
                decimals: 8
            }
            horizontalAlignment: TextInput.AlignHCenter
            color: textColor
            background: Rectangle { color: "transparent" }
        }      

        Item {
            // spacer item
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle { anchors.fill: parent; color: "transparent" } // to visualize the spacer
        }

        RowLayout{
            Layout.fillWidth: true
            Button {
                Layout.fillWidth: true
                contentItem: Text {
                    text: rainTr("RainGUI", "&Coin Control")
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color:bgColor
                }
                onClicked: {
                    coincontrolDialog.open()
                }
                font: theme.thinFont
            }

            Button {
                Layout.fillWidth: true
                contentItem: Text {
                    text: rainTr("RainGUI", "&Add")
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color:bgColor
                }
                onClicked: sendtoModel.append()               
                font: theme.thinFont
            }

            ComboBox {
                id: assetSelector
                currentIndex:-1
                model: assetListModel
                onActivated: {
                    changesendbalance(currentText)
                }

                delegate: ItemDelegate {
                    width: assetSelector.width
                    contentItem: Text {
                        text: modelData
                        color: textColor
                        font: assetSelector.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    highlighted: assetSelector.highlightedIndex === index
                }

                indicator: Canvas {
                    id: canvas
                    x: assetSelector.width - width - assetSelector.rightPadding
                    y: assetSelector.topPadding + (assetSelector.availableHeight - height) / 2
                    width: 12
                    height: 8
                    contextType: "2d"

                    Connections {
                        target: assetSelector
                        onPressedChanged:  canvas.requestPaint()
                    }

                    onPaint: {
                        context.reset();
                        context.moveTo(0, 0);
                        context.lineTo(width, 0);
                        context.lineTo(width / 2, height);
                        context.closePath();
                        context.fillStyle = textColor;
                        context.fill();
                    }
                }

                contentItem: Text {
                    leftPadding: 5
                    rightPadding: assetSelector.indicator.width + assetSelector.spacing

                    text: assetSelector.displayText
                    font: assetSelector.font
                    color: textColor
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    implicitHeight: 30
                    implicitWidth: 120
                    color:"transparent"
                    border.color: textColor
                    border.width: assetSelector.visualFocus ? 2 : 1
                    radius: 2
                }

                popup: Popup {
                    y: assetSelector.height - 1
                    width: assetSelector.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: assetSelector.popup.visible ? assetSelector.delegateModel : null
                        currentIndex: assetSelector.highlightedIndex

                        ScrollIndicator.vertical: ScrollIndicator { }
                    }

                    background: Rectangle {
                        color:"transparent"
                        border.color: tertiaryColor
                        radius: 2
                    }
                }
            }
            Text {
                id: unitLabel
                color: textColor
                text: availableUnits[displayUnit]
                verticalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            id: feeRow
            spacing: 15
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Slider {
                id: feeSlider
                snapMode: Slider.SnapAlways
                stepSize: 1.0 / 8
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true

                background: Rectangle {
                    x: feeSlider.leftPadding
                    y: feeSlider.topPadding + feeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: feeSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: bgColor

                    Rectangle {
                        width: feeSlider.visualPosition * parent.width
                        height: parent.height
                        color: bgColor
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: feeSlider.leftPadding + feeSlider.visualPosition * (feeSlider.availableWidth - width)
                    y: feeSlider.topPadding + feeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: bgColor
                    border.color: textColor
                }

                onMoved: {
                    if (value === 0) {
                        sendPane.confirmations = 1
                    }
                    else if (value === stepSize) {
                        sendPane.confirmations = 2
                    }
                    else if (value === stepSize * 2) {
                        sendPane.confirmations = 4
                    }
                    else if (value === stepSize * 3) {
                        sendPane.confirmations = 6
                    }
                    else if (value === stepSize * 4) {
                        sendPane.confirmations = 8
                    }
                    else if (value === stepSize * 5) {
                        sendPane.confirmations = 10
                    }
                    else if (value === stepSize * 6) {
                        sendPane.confirmations = 12
                    }
                    else if (value === stepSize * 7) {
                        sendPane.confirmations = 14
                    }
                    else if (value === 1) {
                        sendPane.confirmations = 16
                    }
                    setcoincontroltarget(sendPane.confirmations)
                }
            }

            Text {
                id: feeExplainer
                font: theme.thinFont
                text: qsTr("Confirms in ") + sendPane.confirmationsString
                Layout.fillWidth: true
                verticalAlignment: Text.AlignHCenter
                color: textColor
            }

			CheckBox {
				id: enablerbf
				text: qsTr("Enable Replace-By-Fee")
				checked: false
				onCheckedChanged : setrbf(checked)

				indicator: Rectangle {
					implicitWidth: 18
					implicitHeight: 18
					x: enablerbf.leftPadding
					y: parent.height / 2 - height / 2
					radius: 3
					border.color: textColor
					color: "transparent"

					Rectangle {
						width: 12
						height: 12
						x: 3
						y: 3
						radius: 2
						color: textColor
						visible: enablerbf.checked
					}
				}

				contentItem: Text {
					text: enablerbf.text
					font: enablerbf.font
					opacity: enabled ? 1.0 : 0.3
					color: textColor
					verticalAlignment: Text.AlignVCenter
					leftPadding: enablerbf.indicator.width + enablerbf.spacing
				}
			}

        }

        RowLayout {
            Layout.fillWidth: true
            
            RowLayout{
                Layout.fillWidth: true
                Text {
                    color:textColor
                    text: "Balance"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                Text {
                    id:sendpagebalance
                    color:textColor
                    text: "0.0000"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

            }
            
            RowLayout{
                Layout.fillWidth: true
                Text {
                    color:textColor
                    text: "Remaining"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                Text {
                    id:sendpageremaining
                    color:textColor
                    text: "0.0000"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }
        }

        Button {
            contentItem: Text {
                text: rainTr("RainGUI", "&Send")
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Layout.alignment: Qt.AlignHCenter
            background: Rectangle {
                color:bgColor
            }
            onClicked: {
                confirmationDialog.open()
            }
            font: theme.thinFont
        }
    }

    Timer {
        interval: 10000;
        running: true;
        repeat: true
        onTriggered: {
            //coincontrolmodel.update()
            coinView.forceLayout();
        }
    }

    Dialog {
        id: coincontrolDialog
        x: Math.round((parent.width - width) / 2)
        y: Math.round(parent.height / 6)
        width: Math.round(Math.min(parent.width, parent.height) / 10 * 9)
        modal: true
        focus: true
        background: Rectangle {
            anchors.fill: parent
            color : bgColor
        }
        contentItem: Rectangle {
            color:"transparent"
            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    Layout.fillWidth: true
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            font: theme.thinFont
                            color: textColor
                            text: "Selected"
                            Layout.fillWidth: true
                        }

                        Text {
                            id:selctednum
                            font: theme.thinFont
                            color: textColor
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            font: theme.thinFont
                            color: textColor
                            text: "Fee"
                            Layout.fillWidth: true
                        }

                        Text {
                            id:fee
                            font: theme.thinFont
                            color: textColor
                            Layout.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            font: theme.thinFont
                            color: textColor
                            text: "Bytes"
                            Layout.fillWidth: true
                        }

                        Text {
                            id:bytes
                            font: theme.thinFont
                            color: textColor
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            font: theme.thinFont
                            color: textColor
                            text: "After Fee"
                            Layout.fillWidth: true
                        }

                        Text {
                            id:afterfee
                            font: theme.thinFont
                            color: textColor
                            Layout.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    ToolButton {
                        Layout.fillWidth: true
                        contentItem: Text {
                            text: rainTr("RainGUI", "&Select All")
                            color: textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        Layout.alignment: Qt.AlignHCenter
                        background: Rectangle {
                            color:bgColor
                        }
                        onClicked: {
                            confirmationDialog.open()
                        }
                        font: theme.thinFont
                    }
                }

                ListView {
                    id: coinView
                    Layout.fillWidth: true
                    height:200
                    model: coincontrolmodel

                    contentWidth: width
                    clip: true

                    header: RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottomMargin:5

                        Text {
                            horizontalAlignment: Text.AlignHCenter
                            color: textColor
                            width: parent.width / 8
                        }
                        Text {
                            id:addrheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Address")
                            color: textColor
                            width: parent.width / 4
                        }
                        Text {
                            id:assetheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Asset")
                            color: textColor
                            width: parent.width / 4
                        }
                        Text {
                            id:amntheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Amount")
                            color: textColor
                            width: parent.width / 4
                        }
                        Text {
                            horizontalAlignment: Text.AlignHCenter
                            color: textColor
                            width: parent.width / 8
                        }
                    }
                    
                    delegate: RowLayout {
                        //color: "transparent"
                        width: coinView.width
                        anchors.margins:2
                        property int nindex: model.nindex
                        property string txhash: model.txhash
                        property string date:model.date
                        property int confirms: model.confirms
                        property string label:model.label
                        property bool locked : model.locked
                        CheckBox {
                            checked: false
                            width: parent.width / 8
                            onCheckedChanged : setcoincontroloutput(txhash, nindex, checked)

                            indicator: Rectangle {
                                implicitWidth: 10
                                implicitHeight: 10
                                x: parent.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 2
                                border.color: textColor
                                color: "transparent"

                                Rectangle {
                                    width: 5
                                    height: 5
                                    x: 2
                                    y: 2
                                    radius: 2
                                    color: textColor
                                    visible: parent.parent.checked
                                }
                            }
                        }

                        Text {
                            text: model.address
                            color: textColor
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width / 4
                        }

                        Text {
                            text: model.asset
                            color: textColor
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            clip: true
                            width: parent.width / 4
                        }

                        Text {
                            text: model.amount
                            color: textColor
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            clip: true
                            width: parent.width / 4
                        }

                        Image {
                            width: parent.width / 8
                            source: model.locked === true ?  "qrc:/icons/lock_closed" : "qrc:/icons/lock_open"
                            sourceSize: Qt.size (14, 14)
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    lockunlock(txhash,nindex)
                                }
                            }
                        }
                    }
                    //ScrollIndicator.horizontal: ScrollIndicator { }
                    //ScrollIndicator.vertical: ScrollIndicator { }
                }
            }
        }
        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            background: Rectangle {
                color: bgColor
            }

            onAccepted: {
                coincontrolDialog.close()
            }

            onRejected: {
                coincontrolDialog.close()
            }
        }
    }

    Dialog {
        id: confirmationDialog
        x: Math.round((parent.width - width) / 2)
        y: Math.round(parent.height / 6)
        width: Math.round(Math.min(parent.width, parent.height) / 3 * 2)
        modal: true
        focus: true
        background: Rectangle {
            anchors.fill: parent
            color : bgColor
        }

        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            background: Rectangle {
                color: bgColor
            }

            onAccepted: {
                var satAmount = 0
                send(addressText.text, amountField.text, assetSelector.currentText)
                confirmationDialog.close()
                addressText.text=""
                amountField.text=""
            }

            onRejected: {
                confirmationDialog.close()
            }
        }

        contentItem: Rectangle {
            color: bgColor
            anchors.fill: parent
            ColumnLayout {
                spacing: 20

                Text {
                    id:confirmSendtoInfo
                    color:textColor
                    wrapMode: Text.WordWrap
                    text:"Sending " + amountField.text + " To " + addressText.text 
                    horizontalAlignment: Qt.AlignHCenter
                    width:parent.width
                }


                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10
                    Label {
                        text: qsTr("Please confirm to proceed:")
                        wrapMode: Text.WordWrap
                        color: textColor
                        horizontalAlignment: Qt.AlignHCenter
                        width:parent.width
                    }
                }
            }
        }
    }

//    Component.onCompleted: {
//        sendtoModel.append()
//    }
}
