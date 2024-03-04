import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    padding: 12
    title: ActivityCounter.running ? ActivityCounter.titleCache(ActivityCounter.projectId) : ""

    property bool available: RestApiSet.walletAddress

    ColumnLayout {
        anchors.fill: parent

        ColumnLayout {
            Label {
                font.pointSize: appTitleSize
                font.bold: true
                text: control.available ? qsTr("Connected wallet") : qsTr("Please Sign In first!")
            }
            Label {
                Layout.fillWidth: true
                enabled: control.available
                font.pointSize: appTipSize
                wrapMode: Text.Wrap
                onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                text: qsTr("<a href='%1'>%2</a>")
                        .arg(RestApiSet.restApiServer + "/user/" + RestApiSet.walletAddress)
                        .arg(RestApiSet.walletAddress)
            }
        }

        ColumnLayout {
            enabled: control.available
            Label {
                font.pointSize: appTitleSize
                font.bold: true
                text: qsTr("My work")
            }
            RowLayout {
                enabled: HttpRequest.status !== HttpRequest.Busy
                ComboBox {
                    id: titleComboBox
                    Layout.fillWidth: true
                    textRole: "title"
                    valueRole: "id"
                    model: RestApiSet.activityList
                    displayText: ~currentIndex ? currentText : (count ? qsTr("Please select a Project") : "")
                    onModelChanged: currentIndex = indexOfValue(ActivityCounter.projectId)
                    onActivated: {
                        ActivityCounter.setTitleCache(currentValue, currentText)
                        ActivityCounter.projectId = currentValue
                    }
                }
                RoundButton {
                    radius: 4
                    icon.source: "qrc:/icon-refresh"
                    onClicked: RestApiSet.requestActivityList()
                }
            }
            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: textArea.font.pixelSize * 7
                clip: true
                contentWidth: width
                contentHeight: textArea.implicitHeight
                TextArea.flickable: TextArea {
                    id: textArea
                    placeholderText: qsTr("Note")
                    wrapMode: Text.WordWrap
                    text: ActivityCounter.textNote
                    onTextChanged: ActivityCounter.textNote = text
                }
                ScrollIndicator.vertical: ScrollIndicator {}
            }
            Label {
                text: qsTr("Today worked: %1").arg(ActivityCounter.timeCount)
            }
        }

        RoundButton {
            Layout.fillWidth: true
            Layout.preferredHeight: appButtonSize
            enabled: control.available && ActivityCounter.projectId
            leftInset: 0
            rightInset: 0
            radius: 4
            checkable: true
            checked: ActivityCounter.running
            icon.source: checked ? "qrc:/image-stop" : "qrc:/image-start"
            icon.color: enabled ? "transparent" : "gray"
            icon.width: availableHeight
            icon.height: availableHeight
            text: checked ? qsTr("Stop timer") : qsTr("Start timer")
            onToggled: ActivityCounter.running = checked
        }
    }

    footer: Pane {
        padding: 4
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Presses %1").arg(ActivityCounter.keyPresses)
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Clicks %2").arg(ActivityCounter.mouseClicks)
            }
            Label {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Distance %3").arg(ActivityCounter.mouseDistance)
            }
        }
    }
}
