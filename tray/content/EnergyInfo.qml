import QtQuick

EnergyInfoForm {
    Component.onCompleted: function() {
        for (let i = 0; i < energyValuesModel.count; ++i) {
            totalEnergy += energyValuesModel.get(i).enrg
        }
    }
}
