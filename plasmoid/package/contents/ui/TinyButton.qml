import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ToolButton {
    property int iconSize: units.iconSizes.small
    property bool paddingEnabled: false

    // define our own icon property and use that within the style to be able to set QIcon a directly
    // note: The iconSource alias defined in org/kde/plasma/components/ToolButton.qml seems to break
    // passing a QIcon as iconSource (instead of just the name of an icon) so it can not be used.
    property var icon

    style: TinyButtonStyle {}
}
