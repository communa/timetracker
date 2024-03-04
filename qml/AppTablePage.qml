import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt.labs.qmlmodels 1.0

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: ActivityCounter.isTitleCache(currentProject) ? ActivityCounter.titleCache(currentProject)
                                                        : qsTr("Active projects")

    readonly property string myClassName: control.toString().match(/.+?(?=_)/)[0]
    readonly property alias menu:       popupMenu
    readonly property int minColWidth:  Math.ceil(timeTextMetrics.advanceWidth + appTextPadding * 2)
    readonly property int numColWidth:  Math.ceil(numTextMetrics.advanceWidth)
    readonly property int timeColWidth: Math.max(minColWidth, availableWidth - numColWidth * 3)
    readonly property int rowHeight:    appRowHeight
    readonly property color altColor:   MaterialSet.themeColor[Material.theme]["highlight"]
    readonly property int xVelocity:    700 // horizontal pixels in seconds
    readonly property int yVelocity:    900 // vertical pixels in seconds
    readonly property int pageVelocity: 2000 // vertical pixels in seconds
    readonly property var sqlDatePeriod: [
        { "text": QT_TR_NOOP("Day"),   "modifier": "start of day",  "format": "%Y-%m-%d %H:%M" },
        { "text": QT_TR_NOOP("Week"),  "modifier": "-7 days",       "format": "%Y-%m-%d %H:%M" },
        { "text": QT_TR_NOOP("Month"), "modifier": "-1 months",     "format": "%Y-%m-%d" },
        { "text": QT_TR_NOOP("Year"),  "modifier": "-1 years",      "format": "%Y-%m" },
    ]
    property string currentProject
    property bool aggregateTime: true

    function doEscape() : bool {
        if (!currentProject) return false
        currentProject = ""
        makeDbQuery()
        return true
    }

    function columnKey(id, total) : string {
        var key = sqlTableModel.columnIdName(id)
        return total ? "TOTAL(" + key + ") AS " + key : key
    }

    function makeDbQuery() {
        if (!RestApiSet.walletAddress || tabBar.currentIndex < 0 || tabBar.currentIndex >= sqlDatePeriod.length)
            return
        var query = "SELECT "
        if (!control.currentProject) {
            query += columnKey(SqliteProducer.ProjectId)
        } else {
            query += "strftime('" + sqlDatePeriod[tabBar.currentIndex].format
            if (tabBar.currentIndex === 1) {
                if (aggregateTime) {
                    query += "',LocalTime/3600*3600+3600,'unixepoch','localtime')"
                } else {
                    query += "',LocalTime/600*600+600,'unixepoch','localtime')"
                }
            } else if (tabBar.currentIndex === 0 && aggregateTime) {
                query += "',LocalTime/600*600+600,'unixepoch','localtime')"
            } else {
                query += "',LocalTime,'unixepoch','localtime')"
            }
        }
        query += " AS column0, COUNT(*) AS minutesActive"
        query += ", " + columnKey(SqliteProducer.KeyPresses, true)
        query += ", " + columnKey(SqliteProducer.MouseClicks, true)
        query += ", " + columnKey(SqliteProducer.MouseDistance, true)
        query += " FROM '" + RestApiSet.walletAddress + "'"
        query += " WHERE datetime(" + columnKey(SqliteProducer.LocalTime) + ", 'unixepoch','localtime')"
        query += " BETWEEN datetime('now','" + sqlDatePeriod[tabBar.currentIndex].modifier + "','localtime')"
        query += " AND datetime('now','localtime')"
        if (control.currentProject) {
            query += " AND " + columnKey(SqliteProducer.ProjectId) + " IS '" + control.currentProject + "'"
        }
        query += " GROUP BY column0"

        RestApiSet.saveLogFile(query)
        sqlTableModel.view = query
    }

    TextMetrics {
        id: timeTextMetrics
        font: control.font
        text: "0000-00-00 00:00"
    }

    TextMetrics {
        id: numTextMetrics
        font: control.font
        text: "2147483647" // max integer value in QML
    }

    FontMetrics {
        id: fontMetrics
        font: control.font
    }

    ActivityTableModel {
        id: sqlTableModel
        onViewChanged: {
            tableView.forceLayout()
            tableView.contentY = 0
        }
    }

    ButtonGroup {
        id: headerButtonGroup
    }

    header: Column {
        HorizontalHeaderView {
            x: tableView.x
            syncView: tableView
            model: [
                control.currentProject ? QT_TR_NOOP("Time") : QT_TR_NOOP("Work"),
                QT_TR_NOOP("Minutes"), QT_TR_NOOP("Keyboard"), QT_TR_NOOP("Mouse"), QT_TR_NOOP("Distance")
            ]
        }
        ProgressBar {
            width: parent.width
            indeterminate: sqlTableModel.busy
            value: sqlTableModel.view && !sqlTableModel.lastError ? 1.0 : 0.0
        }
    }

    Component {
        id: buttonComponent
        ItemDelegate {
            highlighted: true
            padding: appTextPadding
            text: model.display
            ToolTip.visible: hovered && fontMetrics.advanceWidth(text) >= availableWidth - 2
            ToolTip.text: model.display
            ToolTip.delay: appTipDelay
            ToolTip.timeout: appTipTimeout
            onClicked: {
                control.currentProject = sqlTableModel.rawData(row, column)
                if (control.currentProject) makeDbQuery()
            }
        }
    }

    Component {
        id: textComponent
        Label {
            padding: 0
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: model.display
            elide: Text.ElideRight
            background: Rectangle { color: control.altColor }
        }
    }

    TableView {
        id: tableView
        anchors.fill: parent
        enabled: !sqlTableModel.busy
        focus: true
        clip: true

        columnSpacing: 1
        rowSpacing: 1
        columnWidthProvider: function(column) { return column ? numColWidth : timeColWidth }
        rowHeightProvider: function() { return control.rowHeight }

        model: sqlTableModel
        delegate: DelegateChooser {
            DelegateChoice {
                column: 0
                delegate: control.currentProject ? textComponent : buttonComponent
            }
            DelegateChoice { delegate: textComponent }
        }

        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.OvershootBounds
        transform: [
            Scale {
                origin.x: tableView.width / 2
                origin.y: tableView.verticalOvershoot < 0 ? tableView.height : 0
                yScale: 1.0 - Math.abs(tableView.verticalOvershoot / 10) / tableView.height
            },
            Rotation {
                axis { x: 0; y: 1; z: 0 }
                origin.x: tableView.width / 2
                origin.y: tableView.height / 2
                angle: -Math.min(10, Math.max(-10, tableView.horizontalOvershoot / 10))
            }
        ]
        Keys.onPressed: function(event) {
            if (!tableView.rows) {
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
        ScrollBar.horizontal: ScrollBar { z: 2; active: tableView.rows }
        ScrollBar.vertical: ScrollBar { z: 3; active: tableView.rows }
    }

    footer: TabBar {
        id: tabBar
        position: TabBar.Footer
        Repeater {
            model: sqlDatePeriod
            TabButton {
                text: qsTranslate(control.myClassName, modelData.text)
                background: Rectangle {
                    color: index === tabBar.currentIndex ? control.altColor : "transparent"
                }
            }
        }
        onCurrentIndexChanged: makeDbQuery()
    }

    Label {
        anchors.centerIn: parent
        visible: !tableView.rows
        font.pointSize: appExplainSize
        text: tabBar.currentItem ? qsTr("No data for the %1").arg(tabBar.currentItem.text) : ""
    }

    Menu {
        id: popupMenu
        margins: appTextPadding
        MenuItem {
            enabled: control.currentProject && tabBar.currentIndex < 2
            checkable: true
            checked: control.aggregateTime
            text: qsTr("Aggregate time")
            onToggled: {
                control.aggregateTime = checked
                makeDbQuery()
            }
        }
    }
}
