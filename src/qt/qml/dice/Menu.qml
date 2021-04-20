import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

Rectangle {
    id: gameMenu;
    objectName: "gameMenu"

    signal requestAddr(string gamename)
    signal setupBets(string useraddress, string aiaddress, string amount, string assetname, string nNonce)

    color: "transparent";
    property int numPlayers: 2;
    property int nNonce: 0; 
    
    property var humanList: [true, false, false, false, false, false, false, false];
    property string userAddress : ""
    property string aiAddress: ""
    property string gameName: "diceWars"
    property string gameToken: "Masternode"

	function setAddresses(address, oaddress) {
		gameMenu.userAddress = address
        gameMenu.aiAddress = oaddress
	}

	function setupGame(){
        if(gameMenu.userAddress == "" || gameMenu.aiAddress == "")
            gameMenu.requestAddr(gameMenu.gameName);
        gameMenu.nNonce = Math.floor(Date.now() / 1000);

        gameMenu.setupBets(gameMenu.useraddress, gameMenu.aiaddress, betAmount.text, gameMenu.gameToken, gameMenu.nNonce.toString());
	}

    readonly property int maxPlayers: 3;

    Image {
        anchors.fill: parent;
        anchors.leftMargin: -100;
        anchors.rightMargin: -100;
        anchors.bottomMargin: -100;
        anchors.topMargin: -100;
        source: "qrc:/icons/dbackground";
        opacity: 0.3;
        fillMode: Image.PreserveAspectCrop;
    }

    FontLoader {
        id: titleFont;
        source: "qrc:/fonts/unlearn2"
        onStatusChanged: console.log(status);
    }

    Text {
        id: txtTitle;

        anchors.top: parent.top;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.topMargin: 100;

        height: 120;

        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;

        text: "Chain Dice";
        font.bold: false;
        font.pointSize: 90;
        color: textColor;

        font.family: titleFont.name;
    }

    Text {
        id: txtSubtitle;

        anchors.top: txtTitle.bottom;
        anchors.right: parent.right;
        anchors.topMargin: 5;
        anchors.rightMargin: 120;

        text: "Rain Core";

        color: textColor;

        font.pointSize: 30;
        font.bold: false;
        font.family: titleFont.name;
    }

    RowLayout {
		Layout.fillWidth: true
		id:newassetinputs
		spacing:10

        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 20;
        anchors.bottomMargin: 20;
		
		SpinBox {
			id: betAmount
			Layout.fillWidth: true
			value: 0
			stepSize: 1

			property int decimals: 8

			contentItem: TextInput {
				z: 2
				text: betAmount.textFromValue(betAmount.value, betAmount.locale)
				font: betAmount.font
				//font.pixelSize:14
				color: textColor
				selectionColor: "#ffffff"
				selectedTextColor: "#000000"
				horizontalAlignment: Qt.AlignHCenter
				verticalAlignment: Qt.AlignVCenter

				readOnly: !betAmount.editable
				validator: betAmount.validator
				inputMethodHints: Qt.ImhFormattedNumbersOnly
			}


			up.indicator: Rectangle {
				height: parent.height / 2
				anchors.right: parent.right
				anchors.top: parent.top
				width: 20 // Adjust width here
				color: betAmount.up.pressed ? tertiaryColor : "transparent"
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
				color: betAmount.down.pressed ? tertiaryColor : "transparent"
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
				return Number(value).toLocaleString(locale, 'f', betAmount.decimals)
			}

			valueFromText: function(text, locale) {
				return Number.fromLocaleString(locale, text) * 100
			}

			background: Rectangle {
				implicitWidth: newAssetName.width
				implicitHeight: 20

				color: "transparent"
				border.color: betAmount.enabled ? textColor : tertiaryColor
				border.width: betAmount.visualFocus ? 2 : 1
			}

		}

        Button {
            id: btnStart;
            Layout.fillWidth: true

            contentItem: Text {
                text: "Start!";
                color: textColor;
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Layout.alignment: Qt.AlignHCenter
            background: Rectangle {
                color:"transparent";
                border.color: textColor;
                border.width: 2;
                radius: 10;
                width: 100;
                height: 30;
            }
            MouseArea {
                anchors.fill: parent;
                hoverEnabled: true;
                cursorShape: Qt.PointingHandCursor;

                onEntered: {
                    parent.color = tertiaryColor;
                    parent.textColor = textColor;
                }

                onExited: {
                    parent.color = "transparent";
                    parent.textColor = textColor;
                }

                onClicked: {
                    for (var i = 0; i < gameMenu.maxPlayers; i++)
                        gameMenu.humanList[i] = playerOptions.itemAt(i).human;

                    gameMenu.setupGame();
                    
                    gameContents.numPlayers = numPlayers;
                    gameContents.humanList = humanList;
                    gameContents.userAddress = gameMenu.userAddress
                    gameContents.aiAddress = gameMenu.aiAddress
                    gameContents.nNonce = gameMenu.nNonce;

                    diceWindow.currentIndex = 1
                }
            }
            font: theme.thinFont
        }
    }
    Rectangle {
        id: btnRemovePlayer;

        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.horizontalCenterOffset: -40;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 50;
        color:"transparent";

        width: 60;
        height: 60;
        border.color: textColor;
        border.width: 2;
        radius: 30;

        Text {
            anchors.centerIn: parent;
            font.pointSize: 24;
            font.bold: true;
            text: "-";
            color: textColor;
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            cursorShape: Qt.PointingHandCursor;

            onEntered: {
                parent.color = tertiaryColor;
                parent.textColor = textColor;
            }

            onExited: {
                parent.color = "transparent";
                parent.textColor = textColor;
            }

            onClicked: {
                if (gameMenu.numPlayers < 3) return;
                gameMenu.numPlayers--;
                playerOptions.itemAt(gameMenu.numPlayers).visible = false;
            }
        }
    }

    Rectangle {
        id: btnAddPlayer;

        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.horizontalCenterOffset: 40;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 50;
        color:"transparent";

        width: 60;
        height: 60;
        border.color: textColor;
        border.width: 2;
        radius: 30;

        Text {
            anchors.centerIn: parent;
            font.pointSize: 24;
            font.bold: true;
            text: "+";
            color: textColor;
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            cursorShape: Qt.PointingHandCursor;

            onEntered: {
                parent.color = tertiaryColor;
                parent.textColor = textColor;
            }

            onExited: {
                parent.color = "transparent";
                parent.textColor = textColor;
            }

            onClicked: {
                if (gameMenu.numPlayers >= gameMenu.maxPlayers) return;
                gameMenu.numPlayers++;
                playerOptions.itemAt(gameMenu.numPlayers - 1).visible = true;
            }
        }
    }

    /// This grid will gold each player number and its icon
    Grid {
        columns: 4;
        spacing: 50;

        anchors.fill: parent;
        anchors.leftMargin: 100;
        anchors.rightMargin: 100;
        anchors.topMargin: 400;

        Component.onCompleted: {
            for (var i = 0; i < gameMenu.maxPlayers; i++)
            {
                if (i >= gameMenu.numPlayers) playerOptions.itemAt(i).visible = false;
                playerOptions.itemAt(i).human = gameMenu.humanList[i];
            }
        }

        Repeater {
            id: playerOptions;
            model: gameMenu.maxPlayers;

            Item {
                width: 200;
                height: 80;

                property bool human: false;

                Rectangle {
                    id: playerNumber;

                    height: 40;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: parent.left;
                    width: 40;

                    border.color: textColor;
                    border.width: 2;
                    color:"transparent";

                    radius: 20;

                    Text {
                        anchors.fill: parent;

                        horizontalAlignment: Text.AlignHCenter;
                        verticalAlignment: Text.AlignVCenter;
                        text: index + 1;
                        font.pointSize: 20;
                        color:textColor;
                    }
                }


                Image {
                    id: imgPlayer;

                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: playerNumber.right;
                    anchors.leftMargin: 10;

                    width: 70;
                    height: 70;

                    source: "qrc:/icons/Player" + index + "_Dice" + (index%6+1);
                }

                Rectangle {
                    id: btnHuman;

                    anchors.left: imgPlayer.right;
                    anchors.top: parent.top;
                    anchors.topMargin: 5;
                    anchors.leftMargin: 10;

                    property bool mouseOver: false;

                    width: 120;
                    height: 30;
                    border.color: textColor;
                    border.width: 2;
                    radius: 5;
                    color: mouseOver || parent.human ? textColor : tertiaryColor;

                    Text {
                        anchors.centerIn: parent;
                        font.pointSize: 12;
                        font.bold: true;
                        text: "Human";
                        color: parent.mouseOver || parent.parent.human ? tertiaryColor : textColor;
                    }

                    MouseArea {
                        anchors.fill: parent;
                        hoverEnabled: true;
                        cursorShape: Qt.PointingHandCursor;

                        onEntered: {
                            parent.mouseOver = true;
                        }

                        onExited: {
                            parent.mouseOver = false;
                        }

                        onClicked: {
                            parent.parent.human = true;
                        }
                    }
                }

                Rectangle {
                    id: btnCPU;

                    anchors.left: imgPlayer.right;
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: 5;
                    anchors.leftMargin: 10;

                    property bool mouseOver: false;

                    width: 120;
                    height: 30;
                    border.color: "black";
                    border.width: 2;
                    radius: 5;
                    color: mouseOver || !parent.human ? "black" : tertiaryColor;

                    Text {
                        anchors.centerIn: parent;
                        font.pointSize: 12;
                        font.bold: true;
                        text: "Game Master";
                        color: parent.mouseOver || !parent.parent.human ? tertiaryColor : "black";
                    }

                    MouseArea {
                        anchors.fill: parent;
                        hoverEnabled: true;
                        cursorShape: Qt.PointingHandCursor;

                        onEntered: {
                            parent.mouseOver = true;
                        }

                        onExited: {
                            parent.mouseOver = false;
                        }

                        onClicked: {
                            parent.parent.human = false;
                        }
                    }
                }
            }
        }

    }
}

