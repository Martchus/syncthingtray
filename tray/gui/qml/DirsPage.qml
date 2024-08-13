import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Folder overview")
    Layout.fillWidth: true
    Layout.fillHeight: true

    function dataForSource(source,  role) {
        return app.dirModel.data(app.dirModel.index(source.row, source.col), role)
    }

    DirListView {
        mainModel: app.dirModel
    }
}
