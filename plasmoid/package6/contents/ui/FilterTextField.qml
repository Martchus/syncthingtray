import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

PlasmaComponents3.TextField {
    Layout.topMargin: Kirigami.Units.smallSpacing * 2
    Layout.leftMargin: Kirigami.Units.smallSpacing * 2
    Layout.rightMargin: Kirigami.Units.smallSpacing * 2
    Layout.fillWidth: true
    clearButtonShown: true
    visible: explicitelyShown || text !== ""
    property bool explicitelyShown: false
}
