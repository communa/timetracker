import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    topPadding: 12
    leftPadding: 48
    rightPadding: 48
    bottomPadding: 48
    font.pointSize: appTitleSize
    title: qsTr("Welcome to Communa")

    readonly property int logoSize: availableWidth / 2

    header: ProgressBar {
        indeterminate: !RestApiSet.walletAddress && RestApiSet.busy
    }

    ColumnLayout {
        anchors.fill: parent

        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/image-logo128"
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Click the button below to initiate the login process for your account")
        }

        RoundButton {
            Layout.fillWidth: true
            Layout.preferredHeight: appButtonSize
            radius: 4
            text: qsTr("Sign In")
            onClicked: RestApiSet.startAuthenticate()
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            wrapMode: Text.Wrap
            onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            text: qsTr("<a href='https://communa.network/help'>Having troubles signing in?</a>")
        }
    }
}
