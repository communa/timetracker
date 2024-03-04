import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: RestApiSet.loggerFilePath

    readonly property int textPadding: Math.max(Math.ceil(control.font.pixelSize * 0.1), 2)
    readonly property int rowHeight: Math.ceil(fontMetrics.height + control.textPadding * 2)
    readonly property int xVelocity: 700 // horizontal pixels in seconds
    readonly property int yVelocity: 900 // vertical pixels in seconds
    readonly property int pageVelocity: 2000 // vertical pixels in seconds

    Component.onCompleted: {
        RestApiSet.logFileChanged.connect(loggerListModel.refresh)
        loggerListModel.refresh(SystemHelper.loadText(RestApiSet.loggerFilePath))
    }
    Component.onDestruction: {
        RestApiSet.logFileChanged.disconnect(loggerListModel.refresh)
    }

    FontMetrics {
        id: fontMetrics
        font: control.font
    }

    ListModel {
        id: loggerListModel
        function refresh(list) {
            var max = 0
            for (var line of list) {
                var w = Math.ceil(fontMetrics.advanceWidth(line) + control.textPadding * 2)
                if (w > max) max = w
                append({ "text": line })
            }
            if (count) {
                if (max > listView.contentWidth) listView.contentWidth = max
                listView.contentHeight = count * control.rowHeight
                listView.positionViewAtEnd()
            } else {
                listView.contentWidth = 0
                listView.contentHeight = 0
                listView.positionViewAtBeginning()
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        focus: true
        clip: true

        model: loggerListModel
        delegate: Text {
            height: control.rowHeight
            padding: control.textPadding
            font: control.font
            text: modelData
            color: modelData.startsWith("20") ? Material.accent : Material.foreground
            wrapMode: Text.NoWrap
            verticalAlignment: Text.AlignVCenter
        }

        keyNavigationEnabled: false
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.OvershootBounds
        transform: [
            Scale {
                origin.x: listView.width / 2
                origin.y: listView.verticalOvershoot < 0 ? listView.height : 0
                yScale: 1.0 - Math.abs(listView.verticalOvershoot / 10) / listView.height
            },
            Rotation {
                axis { x: 0; y: 1; z: 0 }
                origin.x: listView.width / 2
                origin.y: listView.height / 2
                angle: -Math.min(10, Math.max(-10, listView.horizontalOvershoot / 10))
            }
        ]
        Keys.onPressed: function(event) {
            if (!listView.count) {
                event.accepted = false
                return
            }
            switch (event.key) {
            case Qt.Key_Up:
                if ((event.modifiers & Qt.ControlModifier) && contentY > 0)
                    contentY = 0
                else flick(0, (event.modifiers & Qt.ShiftModifier) ? control.yVelocity * 2 : control.yVelocity)
                break
            case Qt.Key_Down:
                if ((event.modifiers & Qt.ControlModifier) && contentHeight > height)
                    contentY = contentHeight - height
                flick(0, (event.modifiers & Qt.ShiftModifier) ? -control.yVelocity * 2 : -control.yVelocity)
                break
            case Qt.Key_Left:
                if ((event.modifiers & Qt.ControlModifier) && contentX > 0)
                    contentX = 0
                else flick((event.modifiers & Qt.ShiftModifier) ? control.xVelocity * 2 : control.xVelocity, 0)
                break
            case Qt.Key_Right:
                if ((event.modifiers & Qt.ControlModifier) && contentWidth > width)
                    contentX = contentWidth - width
                else flick((event.modifiers & Qt.ShiftModifier) ? -control.xVelocity * 2 : -control.xVelocity, 0)
                break
            case Qt.Key_PageUp:
                flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ?
                          control.pageVelocity * 2 : control.pageVelocity)
                break
            case Qt.Key_PageDown:
                flick(0, (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) ?
                          -control.pageVelocity * 2 : -control.pageVelocity)
                break
            case Qt.Key_Home:
                contentX = 0
                contentY = 0
                break
            case Qt.Key_End:
                contentX = 0
                if (contentHeight > height) contentY = contentHeight - height
                break
            default:
                event.accepted = false
                return
            }
            event.accepted = true
        }
        ScrollBar.horizontal: ScrollBar { z: 2; active: listView.count }
        ScrollBar.vertical: ScrollBar { z: 3; active: listView.count }
    }
}
