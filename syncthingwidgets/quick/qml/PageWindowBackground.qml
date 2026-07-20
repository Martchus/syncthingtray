import QtQuick

Rectangle {
    id: background
    color: background.pageWindow.theming.baseColor(background.pageWindow.palette)
    required property PageWindow pageWindow
}