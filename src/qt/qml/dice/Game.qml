import QtQuick.Controls 2.12
import Hex 1.0
import QtQuick 2.12

Rectangle {
    id: gameContents;
    objectName: "gameContents"

    signal returnToMenu;
    signal restart;
    signal victory(int player, bool human);

    property int numPlayers: 2;
    property int nNonce: 0; 
    property var humanList: [true, false, false, false, false, false, false, false];
    property string userAddress : ""
    property string aiAddress: ""
    
    color: "transparent";

    /// The board of the game itself
    HexGrid {
        id: hexGrid;

        anchors.fill: parent;
        anchors.topMargin: 50;
        anchors.leftMargin: 50;

        gridWidth: 60;
        gridHeight: 40;
        radius: 10;

        numPlayers: parent.numPlayers;
        humanList: parent.humanList;

        numTerritories: 80;
        territorySize: 25;
        
        nNonce : parent.nNonce;

        Component.onCompleted: {
            restartGame();
        }

        onShowAttackResult: {
            statusMessage.text = attack + " vs " + defense + ((attack > defense) ? " - VICTORY!" : " - Defeat...");
        }

        onConnTerrChanged: {
            if (connTerr == 0) {
                playerLabels.itemAt(player).visible = false;
            } else {
                playerLabels.itemAt(player).visible = true;
                playerLabels.itemAt(player).caption = connTerr;
            }
        }

        onPlayerTurnChanged: {
            var index = playerTurn - 1;
            if (index < 0) index = numPlayers - 1;
            // Disable all player lables, activating only the one for the new player
            for (var i = 0; i < parent.numPlayers; i++)
                playerLabels.itemAt(i).color = "transparent";
            playerLabels.itemAt(playerTurn).color = "yellow";
            // Toggle buttons / hints depending on whether the new player is human
            if (isPlayerHuman(playerTurn)) {
                btnEndTurn.visible = true;
                btnAutoMode.visible = true;
                statusMessage.text = "Your turn!";
            } else {
                btnEndTurn.visible = false;
                btnAutoMode.visible = false;
                statusMessage.text = "Opponent's Turn";
            }
        }

        onVictory: {
            gameContents.victory(player, human);
        }

        function restartGame() {
            initializeGrid();
            for (var i = 0; i < hexGrid.numPlayers; i++)
                playerLabels.itemAt(i).human = hexGrid.isPlayerHuman(i);
        }
    }

    /// Simple MouseArea covering the game board to detect all clicks and pass
    /// them through the board itself
    MouseArea {
        anchors.fill: hexGrid;

        onClicked: {
            hexGrid.processClick(mouse.x, mouse.y);
        }
    }

    /// Text block containing useful tips and messages depending on the game status
    Text {
        id: statusMessage;

        anchors.bottom: parent.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.leftMargin: 50;
        height: 50;
        color:textColor;

        text: "";

        horizontalAlignment: Text.AlignHCenter;
        verticalAlignment: Text.AlignVCenter;
        font.pointSize: 24;
    }

    /// Portion of the bottom of the screen which will hold the scores of each player
    Row {
        anchors.bottom: statusMessage.top;
        anchors.bottomMargin: 0;
        anchors.horizontalCenter: parent.horizontalCenter;

        height: 50;

        Repeater {
            id: playerLabels;
            model: hexGrid.numPlayers;

            Rectangle {
                property string caption: "";
                property bool human: false;

                width: 120;
                height: 50;
                anchors.margins: 10;

                radius: 10;
                color: "transparent";

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.leftMargin: 5;

                    width: 40;
                    height: 40;
                    radius: 15;

                    color: "transparent";
                }

                Image {
                    id: playerPic;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.leftMargin: 10;

                    width: 40;
                    height: 40;

                    source: "qrc:/icons/Player" + index + "_Dice" + (index%6+1);
                }

                Text {
                    height: 40;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: playerPic.right;
                    anchors.leftMargin: 10;
                    anchors.right: parent.right;

                    text: parent.caption;
                    color:textColor;

                    font.pointSize: 24;
                }
            }
        }
    }

    /// Button to go back to the main menu
    Rectangle {
        id: btnMainMenu;

        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.leftMargin: 10;
        anchors.bottomMargin: 10;

        width: 120;
        height: 30;
        border.color: textColor;
        border.width: 2;
        radius: 10;
        color:"transparent";

        Text {
            anchors.centerIn: parent;
            font.pointSize: 12;
            font.bold: true;
            text: "Main Menu";
            color: textColor;
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            cursorShape: Qt.PointingHandCursor;


            onEntered: {
                btnMainMenu.color = tertiaryColor;
                btnMainMenu.textColor = textColor;
            }

            onExited: {
                btnMainMenu.color = "transparent";
                btnMainMenu.textColor = textColor;
            }

            onClicked: {
                diceWindow.currentIndex = 0;
            }
        }
    }

    /// Button to restart the game without going through the main menu
    Rectangle {
        id: btnRestart;

        anchors.bottom: parent.bottom;
        anchors.left: btnMainMenu.right;
        anchors.bottomMargin: 10;
        anchors.leftMargin: 10;

        width: 100;
        height: 30;
        border.color: textColor;
        border.width: 2;
        radius: 10;
        color:"transparent";

        Text {
            anchors.centerIn: parent;
            font.pointSize: 12;
            font.bold: true;
            text: "Restart";
            color: textColor;
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            cursorShape: Qt.PointingHandCursor;


            onEntered: {
                btnRestart.color = tertiaryColor;
                btnRestart.textColor = textColor;
            }

            onExited: {
                btnRestart.color = "transparent";
                btnRestart.textColor = textColor;
            }

            onClicked: {
                playerLabels.itemAt(hexGrid.playerTurn).color = "transparent";
                gameContents.restart();
                hexGrid.restartGame();
            }
        }
    }

    /// Button to finish a human turn once they cannot / do not want to perform
    /// any more actions
    Rectangle {
        id: btnEndTurn;

        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 10;
        anchors.bottomMargin: 10;


        width: 120;
        height: 40;
        border.color: textColor;
        border.width: 2;
        radius: 10;
        color:"transparent";

        Text {
            anchors.centerIn: parent;
            font.pointSize: 14;
            font.bold: true;
            text: "End Turn";
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
                hexGrid.endTurn();
            }
        }
    }

    /// Button that, when toggled, causes human turns to be played automatically
    Rectangle {
        id: btnAutoMode;

        anchors.right: btnEndTurn.left;
        anchors.bottom: parent.bottom;
        anchors.rightMargin: 10;
        anchors.bottomMargin: 10;

        width: 100;
        height: 40;
        border.color: textColor;
        border.width: 2;
        radius: 10;
        color:"transparent";

        Text {
            anchors.centerIn: parent;
            font.pointSize: 14;
            font.bold: true;
            text: "Auto";
            color: textColor;
        }

        MouseArea {
            anchors.fill: parent;
            hoverEnabled: true;
            cursorShape: Qt.PointingHandCursor;

            onEntered: {
                btnAutoMode.color = tertiaryColor;
                btnAutoMode.textColor = textColor;
            }

            onExited: {
                btnAutoMode.color = "transparent";
                btnAutoMode.textColor = textColor;
            }

            onClicked: {
                btnAutoMode.visible = false;
                btnEndTurn.visible = false;
                hexGrid.startAITurn();
            }
        }
    }

    /// Hidden button at the top of the screen which, if toggled, will give
    /// human players much more probabilities of winning in territory battles
    Rectangle {
        id: btnCheat;

        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.leftMargin: 300;
        anchors.topMargin: 5;

        property color textColor: "transparent";
        property bool cheating: false;

        width: 200;
        height: 20;
        border.color: cheating ? "black" : "transparent";
        border.width: 2;
        radius: 5;
        color:"transparent";

        Text {
            anchors.centerIn: parent;
            font.pointSize: 8;
            font.bold: false;
            text: gameContents.aiAddress;
            color: textColor;
        }

        Text {
            anchors.centerIn: parent;
            font.pointSize: 8;
            font.bold: false;
            text: gameContents.userAddress;
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
                //parent.cheating = !parent.cheating;
                //hexGrid.cheatMode = parent.cheating;
            }
        }
    }
    /// This controls the game speed. If too fast, it becomes clear that CPU
    /// struggles to keep up
	Slider {
		id: sldSpeed
		snapMode: Slider.SnapAlways
		stepSize: 1.0 / 8
        anchors.rightMargin: 10
        anchors.topMargin: 8
        anchors.right:parent.right

        value: 0;

		background: Rectangle {
			x: sldSpeed.leftPadding
			y: sldSpeed.topPadding + sldSpeed.availableHeight / 2 - height / 2
			implicitWidth: 200
			implicitHeight: 4
			width: sldSpeed.availableWidth
			height: implicitHeight
			radius: 2
			color: bgColor

			Rectangle {
				width: sldSpeed.visualPosition * parent.width
				height: parent.height
				color: bgColor
				radius: 2
			}
		}

		handle: Rectangle {
			x: sldSpeed.leftPadding + sldSpeed.visualPosition * (sldSpeed.availableWidth - width)
			y: sldSpeed.topPadding + sldSpeed.availableHeight / 2 - height / 2
			implicitWidth: 16
			implicitHeight: 16
			radius: 8
			color: bgColor
			border.color: textColor
		}

		onMoved: hexGrid.gameSpeed = Math.pow(10, value);
		
	}


    /// Label showing the game speed next to the control above
    Text {
        anchors.right: sldSpeed.left;
        anchors.rightMargin: 10;
        anchors.verticalCenter: sldSpeed.verticalCenter;

        text: "Game speed:"
        font.pointSize: 10;
        color:textColor;
    }
}
