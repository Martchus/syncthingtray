pragma Singleton
import QtQuick
import QtCore

Settings {
    property bool isDarkTheme: Qt.styleHints.colorScheme === Qt.Dark
}
