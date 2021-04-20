import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    color: "transparent"

    ComboBox {
        id: contact
        model: addressTablemodel
        Layout.fillWidth: true
        currentIndex: -1
        editable: true

        onAccepted: {
            addressText.text=currentText
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

        contentItem: Text {
            leftPadding: 5
            rightPadding: assetSelector.indicator.width + assetSelector.spacing

            text: contact.displayText
            font: contact.font
            color: textColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color:"transparent"
            border.color: contact.pressed ? tertiaryColor : textColor
            border.width: contact.visualFocus ? 2 : 1
            radius: 2
            implicitHeight: 40
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
        id: addressText
        placeholderText: qsTr("Enter Address...")
        placeholderTextColor: textColor
        horizontalAlignment: TextInput.AlignHCenter
        color:textColor
        Layout.fillWidth: true
        height: 30

        background: Rectangle {
            width: parent.width
            height: parent.height
            color: "transparent"
            border.color: addressText.visualFocus ? tertiaryColor : textColor
        }
    }

    RowLayout {

        TextField {
            id: amountField
            Layout.fillWidth: true
            placeholderText: "0"
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
                implicitHeight: 40
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
    }	

}