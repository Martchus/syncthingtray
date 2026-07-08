import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    title: qsTr("Recent changes")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ChangesListView {
        id: changesListView
    }
    property alias model: changesListView.delegateModel
    property StackView stackView
    property var pageWindow
}
