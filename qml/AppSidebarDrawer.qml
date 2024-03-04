import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Drawer {
    id: control
    modal: true

    readonly property list<Action> actionModel: [
        Action {
            icon.source: "qrc:/icon-preferences"
            text: qsTr("Preferences")
            onTriggered: appPage("PreferencesPage.qml")
        },
        Action {
            enabled: RestApiSet.walletAddress
            icon.source: "qrc:/icon-logout"
            text: qsTr("Sign out")
            onTriggered: {
                appWarning(qsTr("Do you wish to log out?"),
                           Dialog.Yes | Dialog.No).accepted.connect(RestApiSet.clearWallet)
            }
        },
        Action {
            enabled: RestApiSet.walletAddress
            icon.source: "qrc:/icon-database"
            text: qsTr("Table")
            onTriggered: appPage("AppTablePage.qml")
        },
        Action {
            icon.source: "qrc:/icon-about"
            text: qsTr("About")
            onTriggered: appPage("AppAboutPage.qml")
        }
    ]

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        header: Pane {
            Material.elevation: 4
            padding: 0
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    Layout.leftMargin: 12
                    Layout.bottomMargin: 12
                    source: "qrc:/image-logo64"
                    text: qsTr("Open timesheets")
                    onClicked: Qt.openUrlExternally(RestApiSet.restApiServer + "/time")
                }
                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 12
                    font.pointSize: appTitleSize
                    text: qsTr("TimeTracker")
                    elide: Text.ElideRight
                }
            }
        }
        model: control.actionModel
        delegate: ItemDelegate {
            width: listView.width
            //padding: appTextPadding
            spacing: appTextPadding
            action: modelData
            /*indicator: Text {
                visible: !isMobile
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                padding: parent.padding
                font: parent.font
                text: SystemHelper.shortcutText(modelData.shortcut)
                color: Material.foreground
            }*/
            onClicked: control.close()
        }
    }
}
