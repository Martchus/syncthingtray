pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import syncthingtrayApp

ComboBox {
    id: control

    font.family: "Titillium Web"
    font.pixelSize: 14
    font.weight: 400
    textRole: "name"

    background: Rectangle {
        border.color: "#DCDCDC"
        border.width: control.visualFocus ? 2 : 1
        color: Constants.accentColor
        implicitHeight: 40
        implicitWidth: 120
        radius: 8
    }

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Constants.primaryTextColor
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    delegate: ItemDelegate {
        id: comboBoxItem
        required property int index
        required property string name

        highlighted: control.highlightedIndex === index
        palette.light: AppSettings.isDarkTheme ? "#000000" : "#DCDCDC"
        width: control.width

        background: Rectangle {
            color: Color.blend(comboBoxItem.down ? palette.midlight : palette.light, palette.highlight, comboBoxItem.visualFocus ? 0.15 : 0.0)
            radius: 8
            visible: comboBoxItem.down || comboBoxItem.highlighted || comboBoxItem.visualFocus
        }
        contentItem: Text {
            color: Constants.primaryTextColor
            elide: Text.ElideRight
            font: control.font
            text: comboBoxItem.name
            verticalAlignment: Text.AlignVCenter
        }
    }

    indicator: Canvas {
        id: canvas

        contextType: "2d"
        height: 8
        width: 12
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = "#898989";
            context.fill();
        }

        Connections {
            function onPressedChanged() {
                canvas.requestPaint();
            }

            target: control
        }
    }

    popup: Popup {
        implicitHeight: contentItem.implicitHeight
        padding: 1
        width: control.width
        y: control.height - 1

        background: Rectangle {
            border.color: "#DCDCDC"
            color: Constants.accentColor
            radius: 8
        }

        contentItem: ListView {
            clip: true
            currentIndex: control.highlightedIndex
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null

            ScrollIndicator.vertical: ScrollIndicator {
            }
        }
    }
}
