import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    Material.theme: app.darkmodeEnabled ? Material.Dark : Material.Light
    Material.accent: Material.LightBlue
    Material.primary: Material.LightBlue
}
