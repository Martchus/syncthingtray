import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.ToolButton {
    property alias tooltip: tooltip.text
    icon.width: units.iconSizes.small
    icon.height: units.iconSizes.small
    PlasmaComponents3.ToolTip {
        id: tooltip
    }
}
