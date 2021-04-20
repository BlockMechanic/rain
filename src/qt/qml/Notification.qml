import QtQuick 2.12
import QtQuick.Dialogs 1.1

MessageDialog {
    id: popup
    title: "May I have your attention please"
    icon:StandardIcon.Information
    text: ""
    Component.onCompleted: visible = true

    onYes: console.log("copied")
    onNo: console.log("didn't copy")
    onRejected: console.log("aborted")
}
