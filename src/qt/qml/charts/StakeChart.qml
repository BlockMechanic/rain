import QtQuick 2.12
import QtCharts 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

Rectangle {
        id: stakeChartPage

        property variant othersSlice: 0
        property date currentDate: new Date()

        signal btnYearsClicked()
        signal btnMonthsClicked()
        signal btnAllClicked()
        signal currentMIndexChanged(string currentText)
        signal currentYIndexChanged(string currentText)

        color:"transparent"

        // cold, delegated and hot
        ChartView {
            id: stakeChart
            title: "Stake Returns"
            titleColor: textColor
            titleFont.pixelSize: 18
            legend.alignment: Qt.AlignBottom
            antialiasing: true
            backgroundColor : "transparent"

            width:parent.width
            height:parent.height

            BarSeries {
                id: stakeSeries
                axisX: BarCategoryAxis { categories: ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]}

                //BarSet { label: "Cold"; values: [2, 2, 3, 4, 5, 6] }
                //BarSet { label: "Delegated"; values: [5, 1, 2, 4, 1, 7] }
                //BarSet { label: "Classic"; values: [3, 5, 8, 13, 5, 8] }
            }
        }

        ToolBar {

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            background: Rectangle {
                anchors.fill: parent
                color : bgColor
            }
            RowLayout {
                //width: parent.width
                ToolButton {
                    id:pushButtonAll
                    Layout.alignment: Qt.AlignHCenter
                    text: "Days"
                    onClicked: stakeChartPage.btnAllClicked()
                }
                ToolButton {
                    id:pushButtonMonths
                    text: "Months"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: stakeChartPage.btnMonthsClicked()
                }
                ToolButton {
                    id:pushButtonYears
                    text: "Years"
                    onClicked: stakeChartPage.btnYearsClicked()
                    Layout.alignment: Qt.AlignRight
                }

                ComboBox {
                    id: comboBoxMonths
                    model: [ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" ]
                    onCurrentIndexChanged: currentMIndexChanged(currentText)
                }
                ComboBox {
                    id: comboBoxYears
                    textRole: "display"
                    onCurrentIndexChanged: currentYIndexChanged(currentText)
                }
            }
        }
        Component.onCompleted: stakeSeries = appWindow.stakeSeries
}
