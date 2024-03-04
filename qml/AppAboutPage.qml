import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0

Page {
    id: control
    padding: 12
    title: qsTr("About")

    Label {
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: appTitleSize
        wrapMode: Text.Wrap
        text: qsTr("<br><p><b>Communa TimeTracker v%1</b><br>for %2 (%3 %4)</p>
<p>A simple tool designed to optimize and automate time management operations for
both remote workers and enterprises, increasing the convenience of remote work</p><br>
<p><i>Copyright (c) 2024 Ivan Proskuryakov, volgodark@gmail.com<br>
Copyright (c) 2024 Vladimir Vorobyev, b800xy@gmail.com</i></p>")
                .arg(Qt.application.version)
                .arg(SystemHelper.platformOS)
                .arg(SystemHelper.kernelType)
                .arg(SystemHelper.kernelVersion)
    }

    footer: Pane {
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                text: qsTr("Running <a href='http://www.qt.io'>Qt %1</a> %2")
                        .arg(qtVersion)
                        .arg(SystemHelper.buildAbi)
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                onLinkHovered: function(link) { SystemHelper.setCursorShape(link ? Qt.PointingHandCursor : -1) }
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                text: qsTr("<a href='https://opensource.org/licenses/mit-license.php'>MIT License</a>")
            }
        }
    }
}
