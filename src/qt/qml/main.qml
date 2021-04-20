import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

import "messages"
import "charts"
import "contacts"
import "dice"

Rectangle {
    id: appWindow
    anchors.fill: parent
    property bool darkMode: false
    property string textColor: "#ffffff"
    property string bgColor: "#222222"
    property string tertiaryColor: "#5FC7D8"
    property string syncStatus: ""
    property string stakeStatus: ""
    property string colStakeStatus: ""
    property int connectionscount: 0
    property int hdStatus: HDStatus
    property int spvStatus: SPVStatus
    property int lockStatus: EncryptionStatus

    color: bgColor

    signal copyToClipboard(string clipboardText)
    signal changeUnit(int unit)
    signal quit()
    signal lockWallet()
    signal toggleNetwork()
    signal unlockWallet(string pass, bool stakingonly)
    signal encrypt(string pass)
    
    onWidthChanged : {
        if(appWindow.width > 500)
            drawer.open()
        if(appWindow.width < 500)
            drawer.close()    
    }

//    Notification {
//        id: popup
//        visible:false
//    }

    Image {
        id:appBg
        anchors.fill: parent
        source: "qrc:/icons/background"
    }

    ShaderEffect {
        anchors.fill: parent
        property variant source: appBg
        property real frequency: 1
        property real amplitude: 0.1
        property real time: 0.0
        NumberAnimation on time {
            from: 0; to: Math.PI*2; duration: 10000; loops: Animation.Infinite
        }
        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform lowp float qt_Opacity;
            uniform highp float frequency;
            uniform highp float amplitude;
            uniform highp float time;
            void main() {
                highp vec2 texCoord = qt_TexCoord0;
                texCoord.y = amplitude * sin(time * frequency + texCoord.x * 6.283185) + texCoord.y;
                gl_FragColor = texture2D(source, texCoord) * qt_Opacity;
            }"
    }

    function rainTr(context, sourceText) {
        var translated = qsTranslate(context, sourceText)
        return translated
               //.replace(translated.match("\((.*?)\)"), "") // Remove text in brackets
               .replace("&", "") // Remove the underscores
    }

    function showInitMessage(msg) {
        initMessage.text = msg
    }

    function hideSplash() {
        stackView.replace(walletPane)
    }

    function changeStatus(){
        changeEncryptionStatus();
        changeHDStatus();
        changeSPVStatus();
    }

    function changeEncryptionStatus(){
        if(lockStatus===0)
            imgLockUnlock.source = "qrc:/icons/lock_open"

        if(lockStatus===1)
            imgLockUnlock.source = "qrc:/icons/lock_closed"

        if(lockStatus===2)
            imgLockUnlock.source = "qrc:/icons/lock_open"

        if(lockStatus===3)
            imgLockUnlock.source = "qrc:/icons/lock_staking"
    }

    function changeHDStatus(){
        if(hdStatus===0)
            imgHDWallet.source = "qrc:/icons/hd_disabled"

        if(hdStatus===1)
            imgHDWallet.source = "qrc:/icons/hd_enabled"
    }

    function changeSPVStatus(){
        if(spvStatus===0)
            imgSpv.source = "qrc:/icons/spv_disabled"

        if(spvStatus===1)
            imgSpv.source = "qrc:/icons/spv_enabled"
    }

    function updateNetwork(num){
        appWindow.connectionscount = num;
        switch(num)
        {
            case 0: imgConnections.source = "qrc:/icons/connect_0"; break;
            case 1: case 2: case 3: imgConnections.source = "qrc:/icons/connect_1"; break;
            case 4: case 5: case 6: imgConnections.source = "qrc:/icons/connect_2"; break;
            case 7: case 8: case 9: imgConnections.source = "qrc:/icons/connect_3"; break;
            default: imgConnections.source = "qrc:/icons/connect_4"; break;
        }
    }

    function setunlockWallet(num){
        unlockWallet()
    }

    FontLoader
    {
        id: robotoThin
        source: "qrc:/fonts/Roboto-Thin.ttf"
    }

    function updateStaking(stakestate, tooltip){

        if(stakestate==false)
            imgStaking.source = "qrc:/icons/staking_off"

        if(stakestate==true)
            imgStaking.source = "qrc:/icons/staking"

        stakeStatus = tooltip;
    }

    function updateBlocks(syncstate, tooltip){

        if(syncstate==false)
            imgSync.source = "qrc:/icons/sync"

        if(syncstate==true)
            imgSync.source = "qrc:/icons/synced"

        syncStatus=tooltip
    }

    QtObject {
        id: theme
        property font thinFont: Qt.font({family: robotoThin.name, styleName: "Thin", pointSize: 12,})
        property font midFont: Qt.font({family: robotoThin.name, styleName: "Normal", pointSize: 14,})
        property font smallFont: Qt.font({family: robotoThin.name, styleName: "Normal", pointSize: 10,})
        property font tinyFont: Qt.font({family: robotoThin.name, styleName: "Normal", pointSize: 8,})
    }

    PageDrawer {
        id: drawer
        objectName: "drawer"

        signal request()

        function showQr(address) {
            receivePane.address = address
            stackView.push(receivePane)
        }

        iconTitle: "Rain Mobile"

        actions: {
            0: function()  { stackView.pop(walletPane) }, //Dash
            1: function()  { stackView.push(sendPane) }, // send
            2: function()  { request() }, // receive
            3: function()  { stackView.push(messagesMain)  }, // Messages
            4: function()  { stackView.push(chartsPane) }, // Charts
            5: function()  { stackView.push(dicePane) }, // Dice
            6: function()  { stackView.push(contactsPane) }, // Contacts

            8: function() { stackView.push(settingsPane) },  // Settings
            9: function() { stackView.push(consolePane) },  // Settings
            10: function() { },  // Help
        }

        items: ListModel {
            id: pagesModel

            ListElement {
              pageTitle: qsTr ("DashBoard")
              pageIcon: "qrc:/icons/home"
            }

            ListElement {
              pageTitle: qsTr ("Send")
              pageIcon: "qrc:/icons/send"
            }

            ListElement {
              pageTitle: qsTr ("Receive")
              pageIcon: "qrc:/icons/receive"
            }

            ListElement {
              pageTitle: qsTr ("Messages")
              pageIcon: "qrc:/icons/messages"
            }

            ListElement {
              pageTitle: qsTr ("Charts")
              pageIcon: "qrc:/icons/charts"
            }

            ListElement {
              pageTitle: qsTr ("CRASH THIS APP")
              pageIcon: "qrc:/icons/crowdfund"
            }

            ListElement {
              pageTitle: qsTr ("Contacts")
              pageIcon: "qrc:/icons/contacts"
            }

            ListElement {
                spacer: true
            }

            //ListElement {
            //  separator: true
            //}

            ListElement {
              pageTitle: qsTr ("Settings")
              pageIcon: "qrc:/icons/options"
            }
            ListElement {
              pageTitle: qsTr ("Console")
              pageIcon: "qrc:/icons/chevron"
            }
            ListElement {
              pageTitle: qsTr ("Help")
              pageIcon: "qrc:/icons/transaction_0"
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 5
        AppControls {
            id: appControls
            Layout.fillWidth: true
            height: 20
            onClose: Qt.quit()
        }

        RowLayout {
            id: pricesWidget
            objectName: "pricesWidget"
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.bottomMargin: 10
            Layout.leftMargin: 10
            Layout.rightMargin: 10

            function updateValue(btcUSD,rainUSD) {
                btcUsdText.text = btcUSD
                usdXlqText.text = rainUSD
            }

            Text {
                id: btcUsdText
                horizontalAlignment: Text.AlignLeft
                //font.family: robotoThin.name
                font.styleName: "Bold"
                font.pointSize: 12
                Layout.fillWidth: true
                color: textColor
            }

            Text {
                id: usdXlqText
                horizontalAlignment: Text.AlignRight
                //font.family: robotoThin.name
                font.styleName: "Bold"
                font.pointSize: 12
                Layout.fillWidth: true
                color: textColor
            }
        }

		Image {
		  source: "qrc:/icons/rain"
		  sourceSize: Qt.size (48, 48)
		  Layout.alignment: Qt.AlignHCenter
		}  

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 5

            // Use this function to reuse translations from QtWidget contexts
            // For mobile-specific strings use the standard qsTr()

            pushEnter: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to:1
                    duration: 300
                }
            }
            pushExit: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 1
                    to:0
                    duration: 300
                }
            }
            popEnter: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to:1
                    duration: 300
                }
            }
            popExit: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 1
                    to:0
                    duration: 300
                }
            }

            ReceivePane {
              id: receivePane
              objectName: "receivePane"
              visible: false
            }

            SendPane {
              id: sendPane
              objectName: "sendPane"
              visible: false
            }

            SettingsPane {
              id: settingsPane
              objectName: "settingsPane"
              visible: false
            }

            ContactsMain {
              id: contactsPane
              objectName: "contactsPane"
              visible: false
            }

            ChartPane {
              id: chartsPane
              objectName: "chartsPane"
              visible: false
            }

            MessagesMain {
              id: messagesMain
              objectName: "messagesMain"
              visible: false
            }

            Dice {
              id: dicePane
              visible: false
            }

            ConsolePane {
              id: consolePane
              objectName: "consolePane"
              visible: false
            }

            WalletPane {
              id: walletPane
              objectName: "walletPane"
            }

            initialItem: Rectangle {
                id: pane
                color: bgColor

                Text {
                  id: rainCoreText
                  text: "Rain"
                  anchors.top: parent.top
                  anchors.horizontalCenter: parent.horizontalCenter
                  anchors.margins: 20
                  font.family: robotoThin.name
                  font.styleName: "Bold"
                  font.pointSize: 30
                  color: textColor
                }

                Image {
                  id: rainLogo
                  width: 100
                  height: 100
                  anchors.centerIn: parent
                  fillMode: Image.PreserveAspectFit
                  source: "qrc:/icons/rain"
                }

                BusyIndicator {
                    id: busyIndication
                  anchors.centerIn: parent
                    contentItem: Item {
                        implicitWidth: 160
                        implicitHeight: 160

                        Item {
                            id: item
                            x: parent.width / 2 - 80
                            y: parent.height / 2 - 80
                            width: 160
                            height: 160
                            opacity: busyIndication.running ? 1 : 0

                            Behavior on opacity {
                                OpacityAnimator {
                                    duration: 250
                                }
                            }

                            RotationAnimator {
                                target: item
                                running: busyIndication.visible && busyIndication.running
                                from: 0
                                to: 360
                                loops: Animation.Infinite
                                duration: 1250
                            }

                            Repeater {
                                id: repeater
                                model: 6

                                Rectangle {
                                    x: item.width / 2 - width / 2
                                    y: item.height / 2 - height / 2
                                    implicitWidth: 10
                                    implicitHeight: 10
                                    radius: 5
                                    color: textColor
                                    transform: [
                                        Translate {
                                            y: -Math.min(item.width, item.height) * 0.5 + 5
                                        },
                                        Rotation {
                                            angle: index / repeater.count * 360
                                            origin.x: 5
                                            origin.y: 5
                                        }
                                    ]
                                }
                            }
                        }
                    }
                }


                Text {
                    id: initMessage
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.margins: 10
                    font.pointSize: 12
                    color: textColor
                }
            }
        }

        Rectangle {

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            implicitHeight: 40
            color : bgColor
            
            RowLayout {
                anchors.fill: parent

                Image {
                    id: imgSync
                    source: "qrc:/icons/sync"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter

                    ToolTip.text: syncStatus
                    ToolTip.visible: ma.containsMouse

                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

                Image {
                    id: imgColdStaking
                    source: "qrc:/icons/cs_staking_off"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter

                    ToolTip.text: "Cold Staking Disabled"
                    ToolTip.visible: maa.containsMouse

                    MouseArea {
                        id: maa
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

                Image {
                    id: imgStaking
                    source: "qrc:/icons/staking_off"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter

                    ToolTip.text: appWindow.stakeStatus
                    ToolTip.visible: maaa.containsMouse

                    MouseArea {
                        id: maaa
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

				Image {
					id: imgConnections
					source: "qrc:/icons/connect_0"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter

					ToolTip.text: appWindow.connectionscount + " connections"
					ToolTip.visible: mb.containsMouse
					
					MouseArea {
						id: mb
						anchors.fill: parent
						hoverEnabled: true
						onClicked: {
							toggleNetwork()
						}
					}
				}

                Image {
                    id: imgHDWallet
                    source: "qrc:/icons/hd_disabled"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter

                    ToolTip.text: "HD Wallet Disabled"
                    ToolTip.visible: mahdw.containsMouse

                    MouseArea {
                        id: mahdw
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

                Image {
                    id: imgSpv
                    source: "qrc:/icons/spv_disabled"
                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter
                    ToolTip.text: "SPV Mode Disabled"
                    ToolTip.visible: maspv.containsMouse

                    MouseArea {
                        id: maspv
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

                Image {
                    id: imgLockUnlock
                    objectName: "imgLockUnlock"
                    source:  "qrc:/icons/lock_closed"

                    sourceSize: Qt.size (26, 26)
                    Layout.alignment: Qt.AlignHCenter
                    
                    ToolTip.text: lockStatus===0 ? "Encrypt Wallet" : lockStatus===1 ? "Unlock Wallet" : "Lock Wallet"
                    ToolTip.visible: malounl.containsMouse

                    MouseArea {
                        id: malounl
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {

                            if(lockStatus===0){
                                askpassphrasedialog.mode = "Encrypt"
                                askpassphraseheader.text = "Encrypt Wallet"
                                passphrase.visible=false
                                newpassphrase.visible=true
                                repeatpassphrase.visible=true
                                askpassphrasedialog.open();
                            }

                            if(lockStatus===1){
                                askpassphrasedialog.mode = "Unlock"
                                askpassphraseheader.text = "Unlock Wallet"
                                passphrase.visible=true
                                newpassphrase.visible=false
                                repeatpassphrase.visible=false
                                askpassphrasedialog.open();
                            }

                            if(lockStatus===2)
                                lockWallet();

                            if(lockStatus===3)
                                lockWallet();
                        }

                        onPressAndHold : {
                            if(lockStatus!==0){
                                askpassphrasedialog.mode = "ChangePass"
                                askpassphraseheader.text = "Change Wallet PassPhrase"
                                askpassphrasedialog.open();
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: askpassphrasedialog
        visible:false
        modal: true
        focus: true
        background: Rectangle {
            anchors.fill: parent
            color : bgColor
        }
        property string mode: ""
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.width, parent.height) / 3 * 2)

        header: Rectangle {
            color : bgColor
            
            Text {
                id:askpassphraseheader
                color:textColor
                text: ""
                verticalAlignment: Text.AlignHCenter
                anchors.fill:parent
            }
        }

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
                    visible: askpassphrasedialog.mode==="Encrypt" ? false : true

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
                    visible: askpassphrasedialog.mode==="Unlock" || askpassphrasedialog.mode==="Decrypt" ? false: true

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
                    visible: askpassphrasedialog.mode==="Unlock" || askpassphrasedialog.mode==="Decrypt" ? false: true

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
                    visible: askpassphrasedialog.mode==="Unlock" ? true: false

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

        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            background: Rectangle {
                color: bgColor
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
    }
}
