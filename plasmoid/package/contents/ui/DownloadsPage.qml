import QtQuick 2.3
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    property alias view: downloadView
    anchors.fill: parent
    objectName: "DownloadsPage"

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        PlasmaComponents.Label {
            text: "TODO: download delegate/model"
        }

        TopLevelView {
            id: downloadView
            model: plasmoid.nativeInterface.downloadModel

            delegate: TopLevelItem {
                RowLayout {
                    PlasmaCore.IconItem {
                        id: listIcon
                        source: fileIcon
                    }
                    PlasmaComponents.Label {
                        height: implicitHeight
                        elide: Text.ElideRight
                        text: name
                    }
                }
            }
        }
    }
}
