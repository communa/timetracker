import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

ApplicationWindow {
    id: appWindow
    minimumWidth: appFixedWidth
    minimumHeight: appFixedHeight
    maximumWidth: appFixedWidth
    maximumHeight: appFixedHeight
    title: Qt.application.displayName
    visible: true

    readonly property int appFixedWidth:    350
    readonly property int appFixedHeight:   450
    readonly property real scrPixelDensity: (isMobile && (Screen.width / Screen.pixelDensity) > 300) ?
                                                Screen.pixelDensity * 2 : Screen.pixelDensity
    readonly property bool appScreenTiny:   (Screen.width / scrPixelDensity) < 120
    readonly property bool appScreenShort:  (Screen.height / scrPixelDensity) < 120 ||
                                                (isMobile && (Screen.height / Screen.width) < 0.6)
    readonly property bool appScreenHuge:   (Screen.width / scrPixelDensity) >= (23.5 * 25.4) // 27" monitor
    readonly property int  appExplainSize:  font.pointSize + 5
    readonly property int  appTitleSize:    font.pointSize + 2 //(font.pointSize >= 12 ? 2 : 1)
    readonly property int  appTipSize:      font.pointSize - 1 //(font.pointSize >= 12 ? 2 : 1)
    readonly property int  appIconSize:     (appToolButton.height * 1.2) & ~1 // make an even number
    readonly property int  appButtonSize:   (appToolButton.height * 1.4) & ~1 // make an even number
    readonly property int  appRowHeight:    Math.ceil(font.pixelSize * 2.5) & ~1 // make an even number
    readonly property int  appTextPadding:  Math.max(Math.ceil(font.pixelSize * 0.25), 2)
    readonly property int  appTipDelay:     750 // milliseconds
    readonly property int  appTipTimeout:   1500 // milliseconds

    property real appOrigFontSize: 0.0
    property int appMaterialTheme: MaterialSet.defaultTheme
    Material.theme:      appMaterialTheme
    Material.accent:     MaterialSet.themeColor[Material.theme]["accent"]
    Material.background: MaterialSet.themeColor[Material.theme]["background"]
    Material.foreground: MaterialSet.themeColor[Material.theme]["foreground"]
    Material.primary:    active ? MaterialSet.themeColor[Material.theme]["primary"]
                                : MaterialSet.themeColor[Material.theme]["shadePrimary"]
/*
    onActiveFocusControlChanged: console.debug("activeFocusControl", activeFocusControl)
    onActiveFocusItemChanged: console.debug("activeFocusItem", activeFocusItem)
*/
    MySettings {
        id: appSettings
        property alias materialTheme: appWindow.appMaterialTheme
        property alias origFontSize: appWindow.appOrigFontSize
    }

    Component.onCompleted: {
        if (appOrigFontSize) {
            var ps = SystemHelper.loadSettings("lastFontSize", 0.0)
            if (ps && ps !== appWindow.font.pointSize) appWindow.font.pointSize = ps
        } else appOrigFontSize = appWindow.font.pointSize

        ActivityCounter.notification.connect(appInfo)
        ActivityCounter.lastErrorChanged.connect(function(text) {
            var dlg = appError(text)
            if (dlg) dlg.accepted.connect(ActivityCounter.start)
        })
        HttpRequest.recvError.connect(appWarning)
        RestApiSet.errorOccurred.connect(appError)
    }

    onClosing: function(close) {
        if (isMobile && appStackView.depth > 1) {
            appStackView.pop(null)
            close.accepted = false
        }
    }

    function appSetDefaults() {
        if (visibility === ApplicationWindow.Windowed) {
            width = appFixedWidth
            height = appFixedHeight
        }
        appMaterialTheme = MaterialSet.defaultTheme
        if (appOrigFontSize) {
            appWindow.font.pointSize = appOrigFontSize
            SystemHelper.saveSettings("lastFontSize", appOrigFontSize)
        }
        ActivityCounter.idleTime = ActivityCounter.IdleTimeDef
    }

    property var appLastDialog
    function destroyLastDialog() {
        if (appLastDialog) {
            appLastDialog.destroy()
            appLastDialog = undefined
        }
    }

    function appDialog(qml, prop = {}) {
        destroyLastDialog()
        appLastDialog = Qt.createComponent(qml).createObject(appWindow, prop)
        if (appLastDialog) {
            appLastDialog.closed.connect(destroyLastDialog)
            appLastDialog.open()
        } else appToast(qsTr("Can't load %1").arg(qml))
        return appLastDialog
    }

    function appError(text, buttons = Dialog.Abort | Dialog.Ignore) {
        if (!text) return null
        var dlg = appDialog("BriefDialog.qml",
                            { "type": BriefDialog.Type.Error, "text": text, "standardButtons": buttons })
        if (dlg && (buttons & Dialog.Abort)) dlg.rejected.connect(Qt.quit)
        return dlg
    }

    function appWarning(text, buttons = Dialog.Ignore) {
        if (!text) return null
        return appDialog("BriefDialog.qml",
                         { "type": BriefDialog.Type.Warning, "text": text, "standardButtons": buttons })
    }

    function appInfo(text, buttons = Dialog.Ok) {
        if (!text) return null
        return appDialog("BriefDialog.qml",
                         { "type": BriefDialog.Type.Info, "text": text, "standardButtons": buttons })
    }

    function appToast(text) {
        toastComponent.createObject(appWindow, { text })
    }

    function appDelay(interval, func, ...args) { // return Timer instance
        return delayComponent.createObject(appWindow, { interval, func, args })
    }
    function appClearDelay(timer) {
        if (timer instanceof Timer) {
            timer.stop()
            timer.destroy()
        } else appToast(qsTr("clearDelay() arg not a Timer"))
    }

    function appPage(qml, prop = {}) { // prevent dups
        if (appStackView.find(function(item) { return (item.objectName === qml) })) return null
        var page = appStackView.push(qml, prop)
        if (!page) appToast(qsTr("Can't load %1").arg(qml))
        else if (page instanceof Page) page.objectName = qml
        else appToast(qsTr("Not a Page instance %1").arg(qml))
        return page
    }

    Action {
        id: appEscapeAction
        enabled: appToolBar.visible
        shortcut: StandardKey.Cancel
        onTriggered: {
            if (HttpRequest.status === HttpRequest.Busy) {
                HttpRequest.cancel()
            } else if (appStackView.depth < 2) {
                appSidebarDrawer.visible = !appSidebarDrawer.visible
            } else if (typeof appStackView.currentItem.doEscape !== "function" ||
                       !appStackView.currentItem.doEscape()) {
                appStackView.pop()
            }
        }
    }
    Action {
        id: appContextAction
        enabled: appStackView.currentItem && appStackView.currentItem.menu instanceof Menu
        icon.source: "qrc:/icon-more"
        shortcut: "Menu"
        onTriggered: appStackView.currentItem.menu.popup(appStackView.currentItem)
    }

    header: ToolBar {
        id: appToolBar
        visible: RestApiSet.walletAddress
        StackLayout {
            anchors.fill: parent
            currentIndex: HttpRequest.status === HttpRequest.Busy ? 1 : 0
            RowLayout {
                spacing: 0
                ToolButton {
                    id: appToolButton
                    focusPolicy: Qt.NoFocus
                    icon.source: appStackView.depth > 1 ? "qrc:/icon-back" : "qrc:/icon-menu"
                    icon.color: HttpRequest.status === HttpRequest.Ready ? "palegreen" :
                               (HttpRequest.status === HttpRequest.Error ? "darkred" : Material.foreground)
                    action: appEscapeAction
                    rotation: -appSidebarDrawer.position * 90
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    style: Text.Raised
                    styleColor: "gray"
                    elide: Text.ElideRight
                    text: appStackView.currentItem ? appStackView.currentItem.title : ""
                }
                ToolButton {
                    focusPolicy: Qt.NoFocus
                    action: appContextAction
                    visible: action.enabled
                }
            }
            RowLayout {
                spacing: 0
                ToolButton {
                    focusPolicy: Qt.NoFocus
                    action: appEscapeAction
                    BusyIndicator {
                        anchors.fill: parent
                        running: HttpRequest.status === HttpRequest.Busy
                    }
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: HttpRequest.url.toString()
                }
            }
        }
    }

    SwipeView {
        anchors.fill: parent
        focus: true
        interactive: false
        orientation: Qt.Vertical
        currentIndex: RestApiSet.walletAddress ? 1 : 0

        AppAuthPage { }
        StackView {
            id: appStackView
            focus: true // turn-on active focus here
            transform: Translate { x: appSidebarDrawer.position * appSidebarDrawer.width }
            initialItem: AppActivityPage { }
        }
    }

    MouseArea {
       anchors.fill: parent
       acceptedButtons: Qt.RightButton
       onPressed: function(mouse) {
           if (appContextAction.enabled) appContextAction.trigger()
           mouse.accepted = appContextAction.enabled
       }
    }

    AppSidebarDrawer {
        id: appSidebarDrawer
        x: 0; y: appWindow.header.height
        width: Math.min(Math.round(appWindow.width / 2), 220)
        height: appWindow.contentItem.height
        interactive: appStackView.depth < 2
    }

    Component {
        id: toastComponent
        ToolTip {
            font.pointSize: appTipSize
            timeout: 2500
            visible: text
            onVisibleChanged: if (!visible) destroy()
        }
    }
    Component {
        id: delayComponent
        Timer {
            property var func
            property var args
            running: true
            repeat: false
            onTriggered: {
                func(...args)
                destroy()
            }
        }
    }
}
