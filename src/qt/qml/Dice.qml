import QtQuick 2.12
import QtQuick.Layouts 1.3

import "dice"

StackLayout {
    id: diceWindow;

    anchors.fill: parent

//    width: 1130;
//    height: 800;
//    minimumWidth: width;
//    minimumHeight: height;
//    maximumWidth: width;
//    maximumHeight: height;

    property int numPlayers : 2;
    property var humanList: [true, false, false, false, false, false, false, false];

    /// This holds different actions that the main menu or the game need to pass
    /// around, especially when moving from one to the other

    Connections {
        id: connGame;

        ignoreUnknownSignals: true;

        onRestart: {
            //gameOverScreen.visible = false;
        }

        onReturnToMenu: {
            diceWindow.currentIndex = 0 
        }

        onVictory: {
            gameOverScreen.message = "PLAYER " + (player+1) + " WON!";
            gameOverScreen.player = player;
            diceWindow.currentIndex = 2
        }
    }

    Menu{
        id: gameMenu;
    }

    Game{
    
        id: gameContents;
    }

    /// A small dialog in the middle showing who won the game
    Rectangle {
        id: gameOverScreen;

        color: "#aaffffff";
        border.color: "black";
        border.width: 5;
        radius: 20;

        property string message: "";
        property int player: -1;

        Text {
            anchors.left: parent.left;
            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            height: 160;

            color: "black";
            font.pointSize: 35;
            font.bold: true;

            text: parent.message;

            horizontalAlignment: Text.AlignHCenter;
            verticalAlignment: Text.AlignVCenter;
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.top: parent.top;
            anchors.topMargin: 40;

            width: 120;
            height: 120;

            source: parent.player >= 0 ? "qrc:/icons/Player" + parent.player + "_Dice" + (parent.player%6+1) : "";
        }

        Rectangle {
            id: btnMainMenu;

            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
            anchors.rightMargin: 10;
            anchors.bottomMargin: 10;

            property color textColor: "black";
            color: "transparent";

            width: 120;
            height: 30;
            border.color: "black";
            border.width: 3;
            radius: 10;

            Text {
                anchors.centerIn: parent;
                font.pointSize: 12;
                font.bold: true;
                text: "Main Menu";
                color: parent.textColor;
            }

            MouseArea {
                anchors.fill: parent;
                hoverEnabled: true;
                cursorShape: Qt.PointingHandCursor;


                onEntered: {
                    parent.color = "black";
                    parent.textColor = "white";
                }

                onExited: {
                    parent.color = "transparent";
                    parent.textColor = "black";
                }

                onClicked: {
                    gameOverScreen.visible = false;
                    diceWindow.currentIndex = 0
                }
            }
        }
    }
}
