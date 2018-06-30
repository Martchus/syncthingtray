// This is a simplified and adjusted version of Plasma's ToolButtonStyle.
// It will make the button only as big as required and allows to disable
// padding. This makes the button a little bit more compact. Additionally,
// the iconSource works also when a menu is present. The ButtonShadow.qml
// file is still used (version from Plasma 5.38.0).
import QtQuick 2.0
import QtQuick.Controls.Styles 1.1 as QtQuickControlStyle
import QtQuick.Layouts 1.1
import QtQuick.Controls.Private 1.0 as QtQuickControlsPrivate

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents

QtQuickControlStyle.ButtonStyle {
    id: style

    property int minimumWidth
    property int minimumHeight
    property bool flat: control.flat !== undefined ? control.flat : !(control.checkable
                                                                      && control.checked)
    property bool controlHovered: control.hovered
                                  && !(QtQuickControlsPrivate.Settings.hasTouchScreen
                                       && QtQuickControlsPrivate.Settings.isMobile)

    label: RowLayout {
        id: buttonContent
        spacing: units.smallSpacing

        Layout.preferredWidth: Math.max(control.iconSize, label.implicitWidth)
        Layout.preferredHeight: Math.max(control.iconSize, label.implicitHeight)

        PlasmaCore.IconItem {
            id: icon
            source: control.iconName || control.iconSource
            Layout.preferredWidth: control.iconSize
            Layout.preferredHeight: control.iconSize
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            active: style.controlHovered
            colorGroup: controlHovered
                        || !flat ? PlasmaCore.Theme.ButtonColorGroup : PlasmaCore.ColorScope.colorGroup
        }

        //NOTE: this is used only to check elements existence
        PlasmaCore.FrameSvgItem {
            id: buttonsurfaceChecker
            visible: false
            imagePath: "widgets/button"
            prefix: style.flat ? ["toolbutton-hover", "normal"] : "normal"
        }

        PlasmaComponents.Label {
            id: label
            Layout.alignment: Qt.AlignVCenter
            text: control.text
            textFormat: Text.StyledText
            font: control.font || theme.defaultFont
            visible: control.text !== ""
            Layout.fillWidth: true
            color: (controlHovered || !flat) && buttonsurfaceChecker.usedPrefix
                   !== "toolbutton-hover" ? theme.buttonTextColor : PlasmaCore.ColorScope.textColor
            horizontalAlignment: icon.valid ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
        }

        PlasmaExtras.ConditionalLoader {
            id: arrow
            when: control.menu !== null
            visible: when
            Layout.preferredWidth: units.iconSizes.small
            Layout.preferredHeight: units.iconSizes.small

            source: Component {
                PlasmaCore.SvgItem {
                    visible: control.menu !== null
                    svg: PlasmaCore.Svg {
                        imagePath: "widgets/arrows"
                        colorGroup: (style.controlHovered || !style.flat)
                                    && buttonsurfaceChecker.usedPrefix
                                    !== "toolbutton-hover" ? PlasmaCore.Theme.ButtonColorGroup : PlasmaCore.ColorScope.colorGroup
                    }
                    elementId: "down-arrow"
                }
            }
        }
    }

    background: Item {
        id: buttonSurface

        Connections {
            target: control
            onHoveredChanged: {
                if (style.controlHovered) {
                    control.z += 2
                } else {
                    control.z -= 2
                }
            }
        }

        ButtonShadow {
            id: shadow
            visible: !style.flat || control.activeFocus
            anchors.fill: parent
            enabledBorders: surfaceNormal.enabledBorders
            state: {
                if (control.pressed) {
                    return "hidden"
                } else if (style.controlHovered) {
                    return "hover"
                } else if (control.activeFocus) {
                    return "focus"
                } else {
                    return "shadow"
                }
            }
        }

        PlasmaCore.FrameSvgItem {
            id: surfaceNormal
            anchors.fill: parent
            imagePath: "widgets/button"
            prefix: style.flat ? ["toolbutton-hover", "normal"] : "normal"

            enabledBorders: {
                if (style.flat || !control.parent
                        || control.parent.width < control.parent.implicitWidth
                        || control.parent.spacing !== 0
                        || !bordersSvg.hasElement(
                            "pressed-hint-compose-over-border")) {
                    if (shadows !== null) {
                        shadows.destroy()
                    }
                    return "AllBorders"
                }

                var borders = new Array()
                if (control.x == 0) {
                    borders.push("LeftBorder")
                    shadow.anchors.leftMargin = 0
                } else {
                    shadow.anchors.leftMargin = -1
                }
                if (control.y == 0) {
                    borders.push("TopBorder")
                    shadow.anchors.topMargin = 0
                } else {
                    shadow.anchors.topMargin = -1
                }
                if (control.x + control.width >= control.parent.width) {
                    borders.push("RightBorder")
                    shadow.anchors.rightMargin = 0
                } else {
                    shadow.anchors.rightMargin = -1
                }
                if (control.y + control.height >= control.parent.height) {
                    borders.push("BottomBorder")
                    shadow.anchors.bottomMargin = 0
                } else {
                    shadow.anchors.bottomMargin = -1
                }

                if (shadows === null) {
                    shadows = shadowsComponent.createObject(buttonSurface)
                }

                return borders.join("|")
            }

            PlasmaCore.Svg {
                id: bordersSvg
                imagePath: "widgets/button"
            }
        }

        PlasmaCore.FrameSvgItem {
            id: surfacePressed
            anchors.fill: parent
            imagePath: "widgets/button"
            prefix: style.flat ? ["toolbutton-pressed", "pressed"] : "pressed"
            enabledBorders: surfaceNormal.enabledBorders
            opacity: 0
        }

        property Item shadows
        Component {
            id: shadowsComponent
            Item {
                anchors.fill: parent

                PlasmaCore.SvgItem {
                    svg: bordersSvg
                    width: naturalSize.width
                    elementId: (buttonSurface.state == "pressed" ? surfacePressed.prefix : surfaceNormal.prefix) + "-left"
                    visible: button.x > 0
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                        margins: 1
                        leftMargin: -1
                    }
                }
                PlasmaCore.SvgItem {
                    svg: bordersSvg
                    width: naturalSize.width
                    elementId: (buttonSurface.state == "pressed" ? surfacePressed.prefix : surfaceNormal.prefix) + "-right"
                    visible: button.x + button.width < button.parent.width
                    anchors {
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                        margins: 1
                        rightMargin: -1
                    }
                }
                PlasmaCore.SvgItem {
                    svg: bordersSvg
                    height: naturalSize.height
                    elementId: (buttonSurface.state == "pressed" ? surfacePressed.prefix : surfaceNormal.prefix) + "-top"
                    visible: button.y > 0
                    anchors {
                        left: parent.left
                        top: parent.top
                        right: parent.right
                        margins: 1
                        topMargin: -1
                    }
                }
                PlasmaCore.SvgItem {
                    svg: bordersSvg
                    height: naturalSize.height
                    elementId: (buttonSurface.state == "pressed" ? surfacePressed.prefix : surfaceNormal.prefix) + "-bottom"
                    visible: button.y + button.height < button.parent.height
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 1
                        bottomMargin: -1
                    }
                }
            }
        }

        state: (control.pressed
                || control.checked ? "pressed" : (style.controlHovered ? "hover" : "normal"))

        states: [
            State {
                name: "normal"
                PropertyChanges {
                    target: surfaceNormal
                    opacity: style.flat ? 0 : 1
                }
                PropertyChanges {
                    target: surfacePressed
                    opacity: 0
                }
            },
            State {
                name: "hover"
                PropertyChanges {
                    target: surfaceNormal
                    opacity: 1
                }
                PropertyChanges {
                    target: surfacePressed
                    opacity: 0
                }
            },
            State {
                name: "pressed"
                PropertyChanges {
                    target: surfaceNormal
                    opacity: 0
                }
                PropertyChanges {
                    target: surfacePressed
                    opacity: 1
                }
            }
        ]

        transitions: [
            Transition {
                // Cross fade from pressed to normal
                ParallelAnimation {
                    NumberAnimation {
                        target: surfaceNormal
                        property: "opacity"
                        duration: 100
                    }
                    NumberAnimation {
                        target: surfacePressed
                        property: "opacity"
                        duration: 100
                    }
                }
            }
        ]

        Component.onCompleted: {
            if (control.paddingEnabled) {
                padding.top = surfaceNormal.margins.top
                padding.left = surfaceNormal.margins.left
                padding.right = surfaceNormal.margins.right
                padding.bottom = surfaceNormal.margins.bottom
            }
        }
    }
}
