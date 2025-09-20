import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

ExpandableListView {
    id: mainView
    model: DevDelegate {
        mainView: mainView
    }
}
