import QtQuick

HumidityInfoForm {
    Component.onCompleted: function() {
        var hmd = []
        var sum = 0
        var count = humidityValuesModel.count
        for (let i = 0; i < count; ++i) {
            sum += humidityValuesModel.get(i).hmd
            hmd.push(humidityValuesModel.get(i).hmd)
        }
        humidityAvg = sum / count
        humidityDiff = 100 * ( humidityValuesModel.get(count-1).hmd /
                              humidityValuesModel.get(count-2).hmd - 1)
        humidityValues = hmd
        isMore = humidityDiff > 0
    }
}
