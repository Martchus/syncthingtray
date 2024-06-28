// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick

ThermostatInfo {

    required property var energyValuesModel

    property int totalEnergy: 0
    property real costOf1KWH: 0.23

    title: qsTr("Energy Usage")
    leftIcon: "images/energy.svg"
    topLabel: qsTr("Total: %1 KWH".arg(totalEnergy))
    bottomLeftLabel: qsTr("Estimated Cost: $%1".arg(totalEnergy * costOf1KWH))
}
