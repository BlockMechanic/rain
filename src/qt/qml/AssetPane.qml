import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
//import AssetTableModel 0.1

Rectangle {
    color:"transparent"

    signal create(string name,string shortname,string addressto,double inputamt,string assettouse,double outputamt,bool transf,bool conv, bool restr, bool limited)
    signal convert(string addressto, double inputamount, string inputasset, double outputamount, string outputasset)
    signal inflate(string address, string amount, string assetname)

    TabBar {
        id: tabBar
        width:parent.width
        background: Rectangle {
            color: bgColor
        }
        TabButton {
            contentItem: Text {
                text: qsTr("Portfolio")
                opacity: tabBar.currentIndex == 0 ? 1.0 : 0.3
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color:bgColor
            }
        }

        TabButton {
            contentItem: Text {
                text: qsTr("Convert")
                opacity: tabBar.currentIndex == 1 ? 1.0 : 0.3
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color:bgColor
            }
        }

        TabButton {
            contentItem: Text {
                text: qsTr("Create")
                opacity: tabBar.currentIndex == 2 ? 1.0 : 0.3
                color: textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                color:bgColor
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        anchors.topMargin:tabBar.height
        currentIndex: tabBar.currentIndex
        Rectangle {
            id: summaryTab
            color: "transparent"

            ColumnLayout {
                id:tbVl
                anchors.fill: parent
                anchors.margins : 10
                spacing: 5

                ListView {
                    id: tableView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: assettable_model

                    contentWidth: width
                    clip: true

                    header: RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        Text {
                            horizontalAlignment: Text.AlignHCenter
                            color: textColor
                            text: qsTr("Asset Name")
                            width: parent.width / 5
                        }
                        Text {
                            id:addrheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Short Name")
                            color: textColor
                            width: parent.width / 5
                        }
                        Text {
                            id:assetheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Balance")
                            color: textColor
                            width: parent.width / 5
                        }
                        Text {
                            id:amntheader
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Network")
                            color: textColor
                            width: parent.width / 5
                        }
                        Text {
                            horizontalAlignment: Text.AlignHCenter
                            color: textColor
                            text: qsTr("Issuer")
                            width: parent.width / 5
                        }
                    }
                    
                    delegate: RowLayout {
                        anchors.left: tableView.left
                        anchors.right: tableView.right
                        Text {
                            text: model.name
                            color: textColor
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            width: parent.width / 5
                        }

                        Text {
                            text: model.symbol
                            color: textColor
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            width: parent.width / 5
                        }

                        Text {
                            text: model.balance
                            color: textColor
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            width: parent.width / 5
                        }

                        Text {
                            text: model.holdings
                            color: textColor
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            width: parent.width / 5
                        }

                        Text {
                            text: model.issuer
                            color: textColor
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            clip:true
                            width: parent.width / 5
                        }
                    }
                    //ScrollIndicator.horizontal: ScrollIndicator { }
                    //ScrollIndicator.vertical: ScrollIndicator { }
                }

                Timer {
                    interval: 10000;
                    running: true;
                    repeat: true
                    onTriggered: {
                        //assettable_model.update()
                        tableView.forceLayout();
                    }
                }

                CheckBox {
                    id: hideZero
                    text: qsTr("Hide Zero Balances")
                    checked: true

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: hideZero.leftPadding
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
                            visible: hideZero.checked
                        }
                    }

                    contentItem: Text {
                        text: hideZero.text
                        font: hideZero.font
                        opacity: enabled ? 1.0 : 0.3
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: hideZero.indicator.width + hideZero.spacing
                    }
                }
            }
        }

        Rectangle {
            id: manageTab
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins : 10
                spacing: 5

                ComboBox {
                    id: convertAddress
                    model: addressTablemodel
                    Layout.fillWidth: true
                    currentIndex: -1
                    onActivated: {
                        changeUnit(index)
                    }

                    delegate: ItemDelegate {
                        width: convertAddress.width
                        contentItem: Text {
                            text: Address
                            color: textColor
                            font: convertAddress.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        highlighted: convertAddress.highlightedIndex === index
                    }

                    indicator: Canvas {
                        id: canvasu
                        x: convertAddress.width - width - convertAddress.rightPadding
                        y: convertAddress.topPadding + (convertAddress.availableHeight - height) / 2
                        width: 12
                        height: 8
                        contextType: "2d"

                        Connections {
                            target: convertAddress
                            onPressedChanged: canvasu.requestPaint()
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

                        text: convertAddress.displayText
                        font: convertAddress.font
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color:"transparent"
                        border.color: convertAddress.pressed ? tertiaryColor : textColor
                        border.width: convertAddress.visualFocus ? 2 : 1
                        radius: 2
                        implicitHeight: 40
                    }

                    popup: Popup {
                        y: convertAddress.height - 1
                        width: convertAddress.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: convertAddress.popup.visible ? convertAddress.delegateModel : null
                            currentIndex: convertAddress.highlightedIndex

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
                    color:textColor
                    text: "From"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing:10

                    SpinBox {
                        id: convertAssetInputAmount
                        Layout.fillWidth: true
                        value: 0
                        stepSize: 1

                        property int decimals: 8

                        contentItem: TextInput {
                            z: 2
                            text: convertAssetInputAmount.textFromValue(convertAssetInputAmount.value, convertAssetInputAmount.locale)
                            font: convertAssetInputAmount.font
                            //font.pixelSize:14
                            color: textColor
                            selectionColor: "#ffffff"
                            selectedTextColor: "#000000"
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter

                            readOnly: !convertAssetInputAmount.editable
                            validator: convertAssetInputAmount.validator
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                        }


                        up.indicator: Rectangle {
                            height: parent.height / 2
                            anchors.right: parent.right
                            anchors.top: parent.top
                            width: 20 // Adjust width here
                            color: convertAssetInputAmount.up.pressed ? tertiaryColor : "transparent"
                            border.color: textColor
                            Text {
                                text: '+'
                                color:textColor
                                anchors.centerIn: parent
                            }
                        }
                        down.indicator: Rectangle {
                            height: parent.height / 2
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            width: 20 // Adjust width here
                            color: convertAssetInputAmount.down.pressed ? tertiaryColor : "transparent"
                            border.color: textColor
                            Text {
                                text: '-'
                                color:textColor
                                anchors.centerIn: parent
                            }
                        }
                        validator: DoubleValidator {
                            top: 21000000
                            bottom: 0
                            decimals: 8
                        }

                        textFromValue: function(value, locale) {
                            return Number(value).toLocaleString(locale, 'f', convertAssetInputAmount.decimals)
                        }

                        valueFromText: function(text, locale) {
                            return Number.fromLocaleString(locale, text) * 100
                        }

                        background: Rectangle {
                            implicitWidth: newAssetName.width
                            implicitHeight: 20

                            color: "transparent"
                            border.color: convertAssetInputAmount.enabled ? textColor : tertiaryColor
                            border.width: convertAssetInputAmount.visualFocus ? 2 : 1
                        }

                    }

                    ComboBox {
                        id: convertAssetInputAsset
                        model: assetListModel
                        Layout.fillWidth: true
                        height:convertAssetInputAmount.height
                        currentIndex: -1
                        onActivated: {
                           // changeUnit(index)
                        }

                        delegate: ItemDelegate {
                            width: convertAssetInputAsset.width
                            contentItem: Text {
                                text: currentIndex === -1 ? "Input Asset" : modelData
                                color: textColor
                                font: convertAssetInputAsset.font
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            highlighted: convertAssetInputAsset.highlightedIndex === index
                        }

                        indicator: Canvas {
                            id: canvassu
                            x: convertAssetInputAsset.width - width - convertAssetInputAsset.rightPadding
                            y: convertAssetInputAsset.topPadding + (convertAssetInputAsset.availableHeight - height) / 2
                            width: 12
                            height: 8
                            contextType: "2d"

                            Connections {
                                target: convertAssetInputAsset
                                onPressedChanged: canvassu.requestPaint()
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
                            rightPadding: convertAssetInputAsset.indicator.width + convertAssetInputAsset.spacing

                            text: convertAssetInputAsset.displayText
                            font: convertAssetInputAsset.font
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            implicitWidth: newAssetSymbol.width
                            implicitHeight: 30
                            color: "transparent"
                            border.color: textColor
                            border.width: convertAssetInputAsset.visualFocus ? 2 : 1
                            radius: 2
                        }

                        popup: Popup {
                            y: convertAssetInputAsset.height - 1
                            width: convertAssetInputAsset.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 1

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: convertAssetInputAsset.popup.visible ? convertAssetInputAsset.delegateModel : null
                                currentIndex: convertAssetInputAsset.highlightedIndex

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
                Text {
                    color:textColor
                    text: "To"
                    verticalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing:10

                    TextField {
                        id: convertAssetOutputAmount
                        placeholderText: "0.0000"
                        placeholderTextColor: textColor
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        validator: DoubleValidator {
                            bottom: 0
                            decimals: 8
                        }
                        horizontalAlignment: TextInput.AlignHCenter
                        color:textColor
                        background: Rectangle {
                            implicitWidth:convertAssetInputAmount.width
                            height: 30
                            color: "transparent"
                            border.color: textColor
                        }
                    }

                    ComboBox {
                        id: convertAssetOutputAsset
                        model: assetListModel
                        currentIndex: -1
                        onActivated: {
                           // changeUnit(index)
                        }

                        delegate: ItemDelegate {
                            width: convertAssetOutputAsset.width
                            contentItem: Text {
                                text: currentIndex === -1 ? "Input Asset" : modelData
                                color: textColor
                                font: convertAssetOutputAsset.font
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            highlighted: convertAssetOutputAsset.highlightedIndex === index
                        }

                        indicator: Canvas {
                            id: canvasr
                            x: convertAssetOutputAsset.width - width - convertAssetOutputAsset.rightPadding
                            y: convertAssetOutputAsset.topPadding + (convertAssetOutputAsset.availableHeight - height) / 2
                            width: 12
                            height: 8
                            contextType: "2d"

                            Connections {
                                target: convertAssetOutputAsset
                                onPressedChanged: canvasr.requestPaint()
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
                            rightPadding: convertAssetOutputAsset.indicator.width + convertAssetOutputAsset.spacing

                            text: convertAssetOutputAsset.displayText
                            font: convertAssetOutputAsset.font
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            implicitWidth: convertAssetInputAsset.width
                            implicitHeight: 30
                            color: "transparent"
                            border.color: textColor
                            border.width: convertAssetOutputAsset.visualFocus ? 2 : 1
                            radius: 2
                        }

                        popup: Popup {
                            y: convertAssetOutputAsset.height - 1
                            width: convertAssetOutputAsset.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 1

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: convertAssetOutputAsset.popup.visible ? convertAssetOutputAsset.delegateModel : null
                                currentIndex: convertAssetOutputAsset.highlightedIndex

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

                Item {
                    // spacer item
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Rectangle { anchors.fill: parent; color: "transparent" } // to visualize the spacer
                }

                Button {
                    Layout.fillWidth: true

                    contentItem: Text {
                        text: rainTr("RainGUI", "&Convert")
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
                        coincontrolDialog.open()
                    }
                    font: theme.thinFont
                }
            }
        }

        Rectangle {
            id: createTab
            color:"transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins : 10
                spacing: 5

                RowLayout {
                    Layout.fillWidth: true
                    id:newAssetnames
                    spacing: 5

                    TextField {
                        id: newAssetName
                        placeholderText: qsTr("Name (limit 10)")
                        placeholderTextColor: textColor
                        horizontalAlignment: TextInput.AlignHCenter
                        color:textColor
                        Layout.fillWidth: true
                        height: 30

                        background: Rectangle {
                            width:  parent.width
                            height: parent.height
                            color: "transparent"
                            border.color: newAssetName.visualFocus ? tertiaryColor : textColor
                            border.width: newAssetName.visualFocus ? 2 : 1
                        }
                    }

                    TextField {
                        id: newAssetSymbol
                        placeholderText: qsTr("Symbol (limit 4)")
                        placeholderTextColor: textColor
                        horizontalAlignment: TextInput.AlignHCenter
                        color:textColor
                        Layout.fillWidth: true
                        height: 30

                        background: Rectangle {
                            width: parent.width
                            height: parent.height
                            color: "transparent"
                            border.color: newAssetSymbol.visualFocus ? tertiaryColor : textColor
                            border.width: newAssetSymbol.visualFocus ? 2 : 1
                        }
                    }
                }

                ComboBox {
                    id: newAssetIssuingAddress
                    model: addressTablemodel
                    Layout.fillWidth: true
                    currentIndex: -1
                    onActivated: {
                        changeUnit(index)
                    }

                    delegate: ItemDelegate {
                        width: newAssetIssuingAddress.width
                        contentItem: Text {
                            text: Address
                            color: textColor
                            font: newAssetIssuingAddress.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                        }
                        highlighted: newAssetIssuingAddress.highlightedIndex === index
                    }

                    indicator: Canvas {
                        id: canvasi
                        x: newAssetIssuingAddress.width - width - newAssetIssuingAddress.rightPadding
                        y: newAssetIssuingAddress.topPadding + (newAssetIssuingAddress.availableHeight - height) / 2
                        width: 12
                        height: 8
                        contextType: "2d"

                        Connections {
                            target: newAssetIssuingAddress
                            onPressedChanged: canvasi.requestPaint()
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

                        text: newAssetIssuingAddress.displayText
                        font: newAssetIssuingAddress.font
                        color: textColor
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color:"transparent"
                        border.color: newAssetIssuingAddress.pressed ? tertiaryColor : textColor
                        border.width: newAssetIssuingAddress.visualFocus ? 2 : 1
                        radius: 2
                        implicitHeight: 40
                    }

                    popup: Popup {
                        y: newAssetIssuingAddress.height - 1
                        width: newAssetIssuingAddress.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: newAssetIssuingAddress.popup.visible ? newAssetIssuingAddress.delegateModel : null
                            currentIndex: newAssetIssuingAddress.highlightedIndex

                            ScrollIndicator.vertical: ScrollIndicator { }
                        }

                        background: Rectangle {
                            color:"transparent"
                            border.color: tertiaryColor
                            radius: 2
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    id:newassetinputs
                    spacing:10

                    SpinBox {
                        id: newAssetInputAmount
                        Layout.fillWidth: true
                        value: 0
                        stepSize: 1

                        property int decimals: 8

                        contentItem: TextInput {
                            z: 2
                            text: newAssetInputAmount.textFromValue(newAssetInputAmount.value, newAssetInputAmount.locale)
                            font: newAssetInputAmount.font
                            //font.pixelSize:14
                            color: textColor
                            selectionColor: "#ffffff"
                            selectedTextColor: "#000000"
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter

                            readOnly: !newAssetInputAmount.editable
                            validator: newAssetInputAmount.validator
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                        }


                        up.indicator: Rectangle {
                            height: parent.height / 2
                            anchors.right: parent.right
                            anchors.top: parent.top
                            width: 20 // Adjust width here
                            color: newAssetInputAmount.up.pressed ? tertiaryColor : "transparent"
                            border.color: textColor
                            Text {
                                text: '+'
                                color:textColor
                                anchors.centerIn: parent
                            }
                        }
                        down.indicator: Rectangle {
                            height: parent.height / 2
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            width: 20 // Adjust width here
                            color: newAssetInputAmount.down.pressed ? tertiaryColor : "transparent"
                            border.color: textColor
                            Text {
                                text: '-'
                                color:textColor
                                anchors.centerIn: parent
                            }
                        }
                        validator: DoubleValidator {
                            top: 21000000
                            bottom: 0
                            decimals: 8
                        }

                        textFromValue: function(value, locale) {
                            return Number(value).toLocaleString(locale, 'f', newAssetInputAmount.decimals)
                        }

                        valueFromText: function(text, locale) {
                            return Number.fromLocaleString(locale, text) * 100
                        }

                        background: Rectangle {
                            implicitWidth: newAssetName.width
                            implicitHeight: 20

                            color: "transparent"
                            border.color: newAssetInputAmount.enabled ? textColor : tertiaryColor
                            border.width: newAssetInputAmount.visualFocus ? 2 : 1
                        }

                    }

                    ComboBox {
                        id: newAssetInputAsset
                        model: assetListModel
                        Layout.fillWidth: true
                        height:newAssetInputAmount.height
                        currentIndex: -1
                        onActivated: {
                           // changeUnit(index)
                        }

                        delegate: ItemDelegate {
                            width: newAssetInputAsset.width
                            contentItem: Text {
                                text: currentIndex === -1 ? "Input Asset" : modelData
                                color: textColor
                                font: newAssetInputAsset.font
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            highlighted: newAssetInputAsset.highlightedIndex === index
                        }

                        indicator: Canvas {
                            id: canvase
                            x: newAssetInputAsset.width - width - newAssetInputAsset.rightPadding
                            y: newAssetInputAsset.topPadding + (newAssetInputAsset.availableHeight - height) / 2
                            width: 12
                            height: 8
                            contextType: "2d"

                            Connections {
                                target: newAssetInputAsset
                                onPressedChanged: canvase.requestPaint()
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
                            rightPadding: newAssetInputAsset.indicator.width + newAssetInputAsset.spacing

                            text: newAssetInputAsset.displayText
                            font: newAssetInputAsset.font
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            implicitWidth: newAssetSymbol.width
                            implicitHeight: 30
                            color: "transparent"
                            border.color: textColor
                            border.width: newAssetInputAsset.visualFocus ? 2 : 1
                            radius: 2
                        }

                        popup: Popup {
                            y: newAssetInputAsset.height - 1
                            width: newAssetInputAsset.width
                            implicitHeight: contentItem.implicitHeight
                            padding: 1

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: newAssetInputAsset.popup.visible ? newAssetInputAsset.delegateModel : null
                                currentIndex: newAssetInputAsset.highlightedIndex

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

                TextField {
                    id: newAssetIssuedAmount
                    Layout.fillWidth: true
                    placeholderText: "0.0000"
                    placeholderTextColor: textColor
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator {
                        bottom: 0
                        decimals: 8
                    }
                    horizontalAlignment: TextInput.AlignHCenter
                    color:textColor
                    background: Rectangle {
                        width: parent.width
                        height: parent.height
                        color: "transparent"
                        border.color: textColor
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    id:newassetproperties

                    Switch {
                        id: newAssetTransferable
                        text: qsTr("Transferable")

                        indicator: Rectangle {
                            width: 36
                            height: 20
                            x: newAssetTransferable.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 10
                            color: newAssetTransferable.checked ? tertiaryColor : "transparent"
                            border.color: newAssetTransferable.checked ? tertiaryColor : textColor

                            Rectangle {
                                x: newAssetTransferable.checked ? parent.width - width : 0
                                width: 20
                                height: 20
                                radius: 10
                                color: newAssetTransferable.down ? textColor : tertiaryColor
                                border.color: newAssetTransferable.checked ? (newAssetTransferable.down ? tertiaryColor : textColor) : textColor
                            }
                        }

                        contentItem: Text {
                            text: newAssetTransferable.text
                            font: newAssetTransferable.font
                            opacity: enabled ? 1.0 : 0.3
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: newAssetTransferable.indicator.width + newAssetTransferable.spacing
                        }
                    }

                    Switch {
                        id: newAssetConvertable
                        text: qsTr("Convertable")

                        indicator: Rectangle {
                            width: 36
                            height: 20
                            x: newAssetConvertable.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 10
                            color: newAssetConvertable.checked ? tertiaryColor : "transparent"
                            border.color: newAssetConvertable.checked ? tertiaryColor : "#cccccc"

                            Rectangle {
                                x: newAssetConvertable.checked ? parent.width - width : 0
                                width: 20
                                height: 20
                                radius: 10
                                color: newAssetConvertable.down ? textColor : tertiaryColor
                                border.color: newAssetConvertable.checked ? (newAssetConvertable.down ? tertiaryColor : textColor) : textColor
                            }
                        }

                        contentItem: Text {
                            text: newAssetConvertable.text
                            font: newAssetConvertable.font
                            opacity: enabled ? 1.0 : 0.3
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: newAssetConvertable.indicator.width + newAssetConvertable.spacing
                        }
                    }
                }

                RowLayout {
                    Switch {
                        id: newAssetRestricted
                        text: qsTr("Restricted")

                        indicator: Rectangle {
                            width: 36
                            height: 20
                            x: newAssetRestricted.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 10
                            color: newAssetRestricted.checked ? tertiaryColor : "transparent"
                            border.color: newAssetRestricted.checked ? textColor : tertiaryColor

                            Rectangle {
                                x: newAssetRestricted.checked ? parent.width - width : 0
                                width: 20
                                height: 20
                                radius: 10
                                color: newAssetRestricted.down ? textColor : tertiaryColor
                                border.color: newAssetRestricted.checked ? (newAssetRestricted.down ? tertiaryColor : textColor) : textColor
                            }
                        }

                        contentItem: Text {
                            text: newAssetRestricted.text
                            font: newAssetRestricted.font
                            opacity: enabled ? 1.0 : 0.3
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: newAssetRestricted.indicator.width + newAssetRestricted.spacing
                        }
                    }

                    Switch {
                        id: newAssetLimited
                        text: qsTr("Limited")

                        indicator: Rectangle {
                            width: 36
                            height: 20
                            x: newAssetLimited.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 10
                            color: newAssetLimited.checked ? tertiaryColor : "transparent"
                            border.color: newAssetLimited.checked ? textColor : tertiaryColor

                            Rectangle {
                                x: newAssetLimited.checked ? parent.width - width : 0
                                width: 20
                                height: 20
                                radius: 10
                                color: newAssetLimited.down ? textColor : tertiaryColor
                                border.color: newAssetLimited.checked ? (newAssetLimited.down ? tertiaryColor : textColor) : textColor
                            }
                        }

                        contentItem: Text {
                            text: newAssetLimited.text
                            font: newAssetLimited.font
                            opacity: enabled ? 1.0 : 0.3
                            color: textColor
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: newAssetLimited.indicator.width + newAssetLimited.spacing
                        }
                    }
                }

                Item {
                    // spacer item
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Rectangle { anchors.fill: parent; color: "transparent" } // to visualize the spacer
                }

                Button {
                    Layout.fillWidth: true

                    contentItem: Text {
                        text: rainTr("RainGUI", "&Create")
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
                        coincontrolDialog.open()
                    }
                    font: theme.thinFont
                }
            }

        }
    }
}
