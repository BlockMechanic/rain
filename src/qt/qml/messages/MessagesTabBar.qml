import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

Controls.TabBar {
    height: 40

    background: Rectangle {
        color: "transparent"
    }

    MessagesTabButton {
        text: "MSGS"
    }

    MessagesTabButton {
        text: "STATUS"
    }

    MessagesTabButton {
        text: "CALLS"
    }
}
