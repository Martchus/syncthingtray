import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtTest
import Main

Item {
    id: root
    anchors.fill: parent

    ObjectConfigPage {
        id: objectConfigPage
        anchors.fill: parent
        title: "Test"
        configObject: root.config
        specialEntries: [
            {key: "deviceIdKey", label: "Device ID", type: "deviceid", desc: "The device ID"},
            {key: "readonlyKey", label: "Read-only value", type: "readonly"},
            {key: "rangeKey", label: "Range key", type: "range", from: 0.0, to: 100.0, suffix: " %"},
            {key: "optionsKey", label: "Options", type: "options", desc: "Some options", options: [
                                {value: "foo", label: "Foo"},
                                {value: "bar", label: "Bar"},
                            ]},
        ]
    }

    property var config: ({
        stringKey: "stringValue",
        numberKey: 42,
        booleanKey: false,
        readonlyKey: "readonlyValue",
        deviceIdKey: "XXX",
        optionsKey: "foo",
        rangeKey: 24,
    })

    TestCase {
        id: testCase
        name: "ObjectConfigTests"

        function initTestCase() {
        }

        function cleanup() {
        }

        function test_objectConfigPage() {
            const model = objectConfigPage.model;
            compare(model.count, Object.keys(root.config).length, "all entries displayed");
        }

        function test_string_type() {
            const view = objectConfigPage.listView;
            view.currentIndex = 4;
            const item = view.currentItem;
            const dialog = item.dialog;

            // rejecting dialog
            item.click();
            verify(dialog.visible, "dialog to edit string is open");
            compare(dialog.title, "String Key", "dialog title set");
            compare(dialog.text, "stringValue", "dialog text set");
            dialog.text = "new string value";
            dialog.reject();
            compare(root.config.stringKey, "stringValue", "new string value not applied after dialog was rejected");

            // accepting dialog
            item.click();
            compare(dialog.text, "stringValue", "dialog text reset");
            dialog.text = "new string value";
            dialog.accept();
            compare(root.config.stringKey, "new string value", "new string value applied after dialog was accepted");
        }

        function test_range_type() {
            const view = objectConfigPage.listView;
            view.currentIndex = 2;
            const item = view.currentItem;
            const dialog = item.dialog;
            const slider = item.slider;
            compare(slider.value, 24, "slider value set");
            compare(slider.from, 0, "slider from set");
            compare(slider.to, 100, "slider to set");
            compare(slider.stepSize, 1, "slider step size set");
            compare(item.labelText, "24 %", "slider label set with suffix");

            // rejecting dialog
            item.click();
            verify(dialog.visible, "dialog to edit range value manually is open");
            compare(dialog.title, "Range key", "dialog title set");
            compare(dialog.text, "24", "dialog text set");
            dialog.text = "50";
            dialog.reject();
            compare(slider.value, 24, "slider value not updated");
            compare(root.config.rangeKey, 24, "new range value not applied after dialog was rejected");

            // accepting dialog
            item.click();
            compare(dialog.text, "24", "dialog text reset");
            dialog.text = "50";
            dialog.accept();
            compare(slider.value, 50, "slider value updated");
            compare(root.config.rangeKey, 50, "new range value applied after dialog was accepted");

            // use slider
            slider.increase();
            compare(root.config.rangeKey, 51, "new range value applied via slider");
        }
    }
}
