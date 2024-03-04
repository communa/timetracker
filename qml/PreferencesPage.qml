import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import CppCustomModules 1.0
import QmlCustomModules 1.0

Page {
    id: control
    title: qsTr("Preferences")

    property bool modified: false

    Component.onCompleted: {
        dropDownView.setIndexEnable(0, true)
        dropDownView.setIndexEnable(1, true, false)
    }

    readonly property var dropDownModel: [
        {   // index 0
            icon: "qrc:/icon-appearance",
            text: QT_TR_NOOP("Appearance"),
            list: [ { text: QT_TR_NOOP("Color theme"), item: colorThemeComponent },
                    { text: QT_TR_NOOP("Font size"), item: fontSizeComponent },
                    { text: QT_TR_NOOP("Language"), item: languageComponent } ]
        },{   // index 1
            icon: "qrc:/icon-gears",
            text: QT_TR_NOOP("Engine"),
            list: [ { text: QT_TR_NOOP("Server"), item: serverComponent },
                    { text: QT_TR_NOOP("Logging"), item: loggingComponent },
                    { text: QT_TR_NOOP("Idle time"), item: idleTimeComponent } ]
        }
    ]

    Component {
        id: headerComponent
        Pane {
            width: control.availableWidth
            RowLayout {
                anchors.fill: parent
                ImageButton {
                    source: "qrc:/image-preferences"
                    text: qsTr("Visit the Communa homepage")
                    onClicked: Qt.openUrlExternally(RestApiSet.homePage())
                }
                Label {
                    Layout.fillWidth: true
                    font.pointSize: appTitleSize
                    wrapMode: Text.Wrap
                    text: qsTr("Customize your preferences")
                }
                RoundButton {
                    icon.source: "qrc:/icon-reset"
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Reset to defaults")
                    ToolTip.delay: appTipDelay
                    ToolTip.timeout: appTipTimeout
                    onClicked: appSetDefaults()
                }
            }
        }
    }

    // Appearance

    property int colorTheme: appMaterialTheme
    Component {
        id: colorThemeComponent
        RowLayout {
            anchors.fill: parent
            RadioButton {
                Layout.fillWidth: true
                text: qsTr("Dark")
                checked: colorTheme === Material.Dark
                onToggled: {
                    if (checked) {
                        colorTheme = Material.Dark
                        control.modified = true
                    }
                }
            }
            RadioButton {
                Layout.fillWidth: true
                text: qsTr("Light")
                checked: colorTheme === Material.Light
                onToggled: {
                    if (checked) {
                        colorTheme = Material.Light
                        control.modified = true
                    }
                }
            }
        }
    }

    TextMetrics {
        id: sliderTextMetrics
        font: control.font
        text: "000"
    }
    property real fontPointSize: appWindow.font.pointSize
    Component {
        id: fontSizeComponent
        RowLayout {
            Slider {
                id: slider
                Layout.fillWidth: true
                from: appOrigFontSize - (appOrigFontSize >= 12 ? 4 : 2)
                to: appOrigFontSize + (appOrigFontSize >= 12 ? 4 : 2)
                stepSize: value >= 12 ? 2 : 1
                snapMode: Slider.SnapAlways
                value: appWindow.font.pointSize
                onMoved: {
                    fontPointSize = value
                    control.modified = true
                }
            }
            Label {
                Layout.preferredWidth: sliderTextMetrics.tightBoundingRect.width
                horizontalAlignment: Text.AlignRight
                font.pointSize: slider.value
                text: Math.round(slider.value)
            }
        }
    }

    Component { //XXX No implemented yet!
        id: languageComponent
        ComboBox {
            enabled: false
            displayText: "en_US"
            //model: Qt.locale().uiLanguages
        }
    }

    // Engine

    Component {
        id: serverComponent
        MyTextField {
            readOnly: true
            text: RestApiSet.restApiServer
        }
    }

    property bool apiLogging: RestApiSet.logging
    Component {
        id: loggingComponent
        RowLayout {
            Switch {
                Layout.fillWidth: true
                checked: RestApiSet.logging
                text: checked ? qsTr("Enabled") : qsTr("Disabled")
                contentItem: Text {
                    leftPadding: parent.indicator.width + parent.spacing
                    text: parent.text
                    font: parent.font
                    color: parent.checked ? Material.accent : Material.foreground
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                onToggled: {
                    control.apiLogging = checked
                    control.modified = true
                }
            }
            RoundButton {
                radius: 4
                icon.source: "qrc:/icon-open-window"
                onClicked: appPage("AppLoggingPage.qml")
            }
        }
    }

    property int idleTime: ActivityCounter.idleTime
    Component {
        id: idleTimeComponent
        SpinBox {
            enabled: false
            from: ActivityCounter.IdleTimeMin
            to: ActivityCounter.IdleTimeMax
            value: idleTime
            stepSize: 5
            textFromValue: function(value) { return value + " minutes" }
            onValueModified: {
                idleTime = value
                control.modified = true
            }
        }
    }

    DropDownView {
        id: dropDownView
        anchors.fill: parent
        myClassName: control.toString().match(/.+?(?=_)/)[0]
        model: control.dropDownModel
        header: headerComponent
    }

    footer: DropDownPane {
        id: dropDownPane
        show: control.modified
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Save the changes?")
                elide: Text.ElideRight
            }
            DialogButtonBox {
                position: DialogButtonBox.Footer
                standardButtons: DialogButtonBox.Ok
                onAccepted: {
                    // prevent blames about binding loop detected for property "implicitHeight"
                    dropDownView.setIndexEnable(0, false)
                    dropDownView.setIndexEnable(1, false)
                    dropDownPane.show = false

                    appMaterialTheme = control.colorTheme

                    if (control.fontPointSize !== appWindow.font.pointSize) {
                        appWindow.font.pointSize = control.fontPointSize
                        if (appWindow.font.pointSize !== appOrigFontSize) {
                            SystemHelper.saveSettings("lastFontSize", appWindow.font.pointSize)
                        } else {
                            SystemHelper.saveSettings("lastFontSize")
                        }
                    }
                    if (control.apiLogging !== RestApiSet.logging) {
                        RestApiSet.logging = control.apiLogging
                        SystemHelper.saveSettings(RestApiSet.myClassName + "/logging", RestApiSet.logging)
                        if (RestApiSet.logging) RestApiSet.startLogFile()
                    }
                    ActivityCounter.idleTime = idleTime
                    appStackView.pop()
                }
            }
        }
    }
}
