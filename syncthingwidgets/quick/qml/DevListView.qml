import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ExpandableListView {
    id: mainView
    mainModel: SyncthingModels.sortFilterDevModel
    model: DevDelegate {
        mainView: mainView
    }
}
