import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import Main

Main {
    Material.theme: app.darkmodeEnabled ? Material.Dark : Material.Light
    Material.accent: Material.LightBlue
}
