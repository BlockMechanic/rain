import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3


Rectangle {
    color:"transparent"

    ColumnLayout {
        id: settingsColumn
        anchors.fill: parent

        Text {
            id: talkccoinCoreText

            anchors.horizontalCenter: parent.horizontalCenter

            text: "Rain<b>Core</b>"
            font.family: robotoThin.name
            font.styleName: "Thin"
            font.pointSize: 14
            color: textColor
        }


        Text {
            id: versionText

            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter

            text: appWindow.version

            font.family: robotoThin.name
            font.styleName: "Thin"
            font.pointSize: 10

            color: textColor
        }

        GroupBox {
            id: gb1
            Layout.fillWidth: true
            title: rainTr("OptionsDialog", "&Display")

            label: Label {
                x: gb1.leftPadding
                width: gb1.availableWidth
                text: gb1.title
                color: textColor
                elide: Text.ElideRight
            }

            ColumnLayout {
                anchors.fill: parent

                RowLayout {
                    spacing: 5

                    Label {
                        text: rainTr("OptionsDialog", "&Unit to show amounts in:")
                        font: theme.thinFont
                        color: textColor
                    }

                    ComboBox {
                        id: unitselector
                        model: availableUnits
                        Layout.fillWidth: true
                        currentIndex: -1
                        onActivated: {
                            changeUnit(index)
                        }

                        delegate: ItemDelegate {
                            width: unitselector.width
                            contentItem: Text {
                                text: currentIndex === -1 ? "Input Asset" : modelData
                                color: textColor
                                font: unitselector.font
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            highlighted: unitselector.highlightedIndex === index
                        }

                        indicator: Canvas {
                            id: canvassss
                            x: unitselector.width - width - unitselector.rightPadding
                            y: unitselector.topPadding + (unitselector.availableHeight - height) / 2
                            width: 12
                            height: 8
                            contextType: "2d"

                            Connections {
                                target: unitselector
                                onPressedChanged: canvassss.requestPaint()
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
                            leftPadding: 0
                            rightPadding: unitselector.indicator.width + unitselector.spacing

                            text: unitselector.displayText
                            font: unitselector.font
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            color: "transparent"
                            border.color: textColor
                            border.width: unitselector.visualFocus ? 2 : 1
                            radius: 2
                            implicitHeight: 40
                        }

                        popup: Popup {
                            y: unitselector.height - 1
                            width: unitselector.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 1

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: unitselector.popup.visible ? unitselector.delegateModel : null
                                currentIndex: unitselector.highlightedIndex

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
                Switch {
                    id: swtdarkmode
                    text: qsTr("Dark Mode")

                    onToggled: {
                        if (checked) {
                            darkMode = true
                        }
                        else {
                            darkMode = false
                        }
                    }

                    indicator: Rectangle {
                        width: 36
                        height: 20
                        x: swtdarkmode.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 10
                        color: swtdarkmode.checked ? tertiaryColor : "transparent"
                        border.color: swtdarkmode.checked ? tertiaryColor : "#cccccc"

                        Rectangle {
                            x: swtdarkmode.checked ? parent.width - width : 0
                            width: 20
                            height: 20
                            radius: 10
                            color: swtdarkmode.down ? textColor : tertiaryColor
                            border.color: swtdarkmode.checked ? (swtdarkmode.down ? tertiaryColor : tertiaryColor) : "#999999"
                        }
                    }

                    contentItem: Text {
                        text: swtdarkmode.text
                        font: swtdarkmode.font
                        opacity: enabled ? 1.0 : 0.3
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: swtdarkmode.indicator.width + swtdarkmode.spacing
                    }
                }
            }
        }

        GroupBox {
            id:gb2
            Layout.fillWidth: true
            title: rainTr("OptionsDialog", "&Secure Messaging")

            label: Label {
                x: gb2.leftPadding
                width: gb2.availableWidth
                text: gb2.title
                color: textColor
                elide: Text.ElideRight
            }

            ColumnLayout {
                anchors.fill: parent

                RowLayout {
                    spacing: 5

                    Label {
                        text: rainTr("OptionsDialog", "&Primary Address:")
                        font: theme.thinFont
                        color: textColor
                    }

                  ComboBox {
                      editable: true
                      id: addressselector
                      Layout.fillWidth: true
                      model: addressTablemodel

                        delegate: ItemDelegate {
                            width: addressselector.width
                            contentItem: Text {
                                text: currentIndex === -1 ? "Enter Address..." : modelData
                                color: textColor
                                font: addressselector.font
                                elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                            }
                            highlighted: addressselector.highlightedIndex === index
                        }

                        indicator: Canvas {
                            id: canvsass
                            x: addressselector.width - width - addressselector.rightPadding
                            y: addressselector.topPadding + (addressselector.availableHeight - height) / 2
                            width: 12
                            height: 8
                            contextType: "2d"

                            Connections {
                                target: addressselector
                                onPressedChanged:  canvsass.requestPaint()
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
                            leftPadding: 0
                            //rightPadding: assetSelector.indicator.width + assetSelector.spacing

                            text: addressselector.displayText
                            font: addressselector.font
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            implicitWidth: 120
                            implicitHeight: 40
                            color:"transparent"
                            border.color: textColor
                            border.width: addressselector.visualFocus ? 2 : 1
                            radius: 2
                        }

                        popup: Popup {
                            y: addressselector.height - 1
                            width: addressselector.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 1

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: addressselector.popup.visible ? addressselector.delegateModel : null
                                currentIndex: addressselector.highlightedIndex

                                ScrollIndicator.vertical: ScrollIndicator { }
                            }

                            background: Rectangle {
                                color:"transparent"
                                border.color: tertiaryColor
                                radius: 2
                            }
                        }

                        onAccepted: {
                            if (find(editText) === -1)
                                model.append({text: editText})
                        }
                    }
                }

                Switch {
                    id: swtdelread
                    text: qsTr("Delete Read Messages")

                    onToggled: {
                        if (checked) {
                            //darkMode = true
                        }
                        else {
                            //darkMode = false
                        }
                    }

                    indicator: Rectangle {
                        width: 36
                        height: 20
                        x: swtdelread.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 10
                        color: swtdelread.checked ? tertiaryColor : "transparent"
                        border.color: swtdelread.checked ? tertiaryColor : "#cccccc"

                        Rectangle {
                            x: swtdelread.checked ? parent.width - width : 0
                            width: 20
                            height: 20
                            radius: 10
                            color: swtdelread.down ? textColor : tertiaryColor
                            border.color: swtdelread.checked ? (swtdelread.down ? tertiaryColor : tertiaryColor) : "#999999"
                        }
                    }

                    contentItem: Text {
                        text: swtdelread.text
                        font: swtdelread.font
                        opacity: enabled ? 1.0 : 0.3
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: swtdelread.indicator.width + swtdelread.spacing
                    }
                }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: licenceText.width
            contentHeight: licenceText.height

            clip: true

            Text {
                id: licenceText

                width: parent.parent.width

                text: appWindow.licenceInfo
                font.pointSize: 10
                color: textColor
                wrapMode: Text.Wrap
            }
        }

    }
}
