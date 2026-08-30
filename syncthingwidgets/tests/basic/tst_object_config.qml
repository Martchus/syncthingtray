import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtTest
import Main

Item {
    id: root
    anchors.fill: parent

    StackView {
        id: myStackView
        anchors.fill: parent
        initialItem: ObjectConfigPage {
            id: objectConfigPage
            title: "Test"
            configObject: root.config
            stackView: myStackView
            specialEntries: [
                {key: "deviceIdKey", label: "Device ID", type: "deviceid", desc: "The device ID"},
                {key: "readonlyKey", label: "Read-only value", type: "readonly"},
                {key: "rangeKey", label: "Range key", type: "range", from: 0.0, to: 100.0, suffix: " %"},
                {key: "optionsKey", label: "Options", type: "options", desc: "Some options", options: [
                                    {value: "foo", label: "Foo"},
                                    {value: "bar", label: "Bar"},
                                ]},
                {key: "devicesKey", label: "Devices", type: "devices"},
                {key: "filepathKey", label: "File Path", type: "filepath"},
                {key: "folderpathKey", label: "Folder Path", type: "folderpath"},
            ]
        }
    }

    property bool functionCalled: false

    property var config: ({
        stringKey: "stringValue",
        numberKey: 42,
        booleanKey: false,
        readonlyKey: "readonlyValue",
        deviceIdKey: "XXX",
        optionsKey: "foo",
        rangeKey: 24,
        devicesKey: [
            {deviceID: "ABC-123", encryptionPassword: "pwd", introducedBy: ""}
        ],
        objectKey: { subKey: "subValue" },
        functionKey: function() { root.functionCalled = true; },
        filepathKey: "/path/to/file",
        folderpathKey: "/path/to/folder",
    })

    TestCase {
        id: testCase
        name: "ObjectConfigTests"

        function initTestCase() {
        }

        function cleanup() {
        }

        function findChild(parent, predicate) {
            if (!parent) return null;
            if (predicate(parent)) return parent;
            if (parent.contentItem) {
                const found = findChild(parent.contentItem, predicate);
                if (found) return found;
            }
            if (parent.children) {
                for (let i = 0; i < parent.children.length; i++) {
                    const found = findChild(parent.children[i], predicate);
                    if (found) return found;
                }
            }
            return null;
        }

        function getDelegateForKey(key) {
            const index = objectConfigPage.indexByKey[key];
            verify(index >= 0, "Key exists: " + key);
            const view = objectConfigPage.listView;
            view.positionViewAtIndex(index, ListView.Center);
            view.currentIndex = index;
            wait(50);
            const item = view.currentItem;
            verify(item !== null, "Item exists for key: " + key);
            console.log("getDelegateForKey key:", key, "index:", index, "item:", item, "modelData.key:", item.modelData ? item.modelData.key : "none", "modelData.type:", item.modelData ? item.modelData.type : "none");
            return item;
        }

        function test_objectConfigPage() {
            const model = objectConfigPage.model;
            compare(model.count, Object.keys(root.config).length, "all entries displayed");
        }

        function test_readonly_type() {
            const item = getDelegateForKey("readonlyKey");
            const label = findChild(item, (c) => c.text !== undefined && c.text === "Read-only value");
            verify(label !== null, "Label is correct");
            const valLabel = findChild(item, (c) => c.text !== undefined && c.text === "readonlyValue");
            verify(valLabel !== null, "Value label is correct");
        }

        function test_string_type() {
            const item = getDelegateForKey("stringKey");
            const dialog = item.dialog;

            // rejecting dialog
            item.clicked();
            verify(dialog.visible, "dialog to edit string is open");
            compare(dialog.title, "String Key", "dialog title set");
            compare(dialog.text, "stringValue", "dialog text set");
            dialog.text = "new string value";
            dialog.reject();
            compare(root.config.stringKey, "stringValue", "new string value not applied after dialog was rejected");

            // accepting dialog
            item.clicked();
            compare(dialog.text, "stringValue", "dialog text reset");
            dialog.text = "new string value";
            dialog.accept();
            compare(root.config.stringKey, "new string value", "new string value applied after dialog was accepted");
        }

        function test_deviceid_type() {
            const item = getDelegateForKey("deviceIdKey");
            const dialog = item.dialog;

            // open dialog
            item.clicked();
            verify(dialog.visible, "dialog to edit device ID is open");
            compare(dialog.title, "Device ID", "dialog title is set");

            // find ComboBox
            const comboBox = findChild(dialog, (c) => c.editText !== undefined);
            verify(comboBox !== null, "ComboBox found");
            compare(comboBox.editText, "XXX", "initial value is correct");

            // change value and reject
            comboBox.editText = "YYY";
            dialog.reject();
            compare(root.config.deviceIdKey, "XXX", "value not updated after rejection");

            // change value and accept
            item.clicked();
            comboBox.editText = "P-Q-R";
            dialog.accept();
            compare(root.config.deviceIdKey, "P-Q-R", "value updated after acceptance");
        }

        function test_options_type() {
            const item = getDelegateForKey("optionsKey");
            const dialog = item.dialog;

            // open dialog
            item.clicked();
            verify(dialog.visible, "options dialog is open");

            const comboBox = findChild(dialog, (c) => c.model !== undefined && c.currentOption !== undefined);
            verify(comboBox !== null, "ComboBox for options found");
            compare(comboBox.editText, "Foo", "displays option label");

            // change value and reject
            comboBox.currentIndex = 1;
            comboBox.editText = "Bar";
            dialog.reject();
            compare(root.config.optionsKey, "foo", "options not updated on reject");

            // change value and accept
            item.clicked();
            comboBox.currentIndex = 1;
            comboBox.editText = "Bar";
            dialog.accept();
            compare(root.config.optionsKey, "bar", "options updated on accept");
        }

        function test_devices_type() {
            const item = getDelegateForKey("devicesKey");
            verify(item !== null, "Devices delegate item found");

            const devicesModel = item.devicesModel;
            verify(devicesModel.length > 0, "devicesModel has entries");
            compare(devicesModel[0].deviceID, "ABC-123", "device ID matches");

            verify(item.isDeviceEnabled("ABC-123"), "device ABC-123 is initially enabled");
            verify(!item.isDeviceEnabled("DEF-456"), "device DEF-456 is not enabled");

            item.setDeviceEnabled("DEF-456", true);
            verify(item.isDeviceEnabled("DEF-456"), "device DEF-456 is now enabled");
            compare(root.config.devicesKey.length, 2, "config has 2 devices now");

            item.setDeviceEnabled("DEF-456", false);
            verify(!item.isDeviceEnabled("DEF-456"), "device DEF-456 is disabled again");
        }

        function test_number_type() {
            const item = getDelegateForKey("numberKey");
            const dialog = item.dialog;

            // open dialog
            item.clicked();
            verify(dialog.visible, "number dialog is open");
            compare(dialog.text, "42", "initial text is 42");

            // change and reject
            dialog.text = "100";
            dialog.reject();
            compare(root.config.numberKey, 42, "value not updated on reject");

            // change and accept
            item.clicked();
            dialog.text = "100";
            dialog.accept();
            compare(root.config.numberKey, 100, "value updated on accept");
        }

        function test_range_type() {
            const item = getDelegateForKey("rangeKey");
            const dialog = item.dialog;
            const slider = item.slider;
            compare(slider.value, 24, "slider value set");
            compare(slider.from, 0, "slider from set");
            compare(slider.to, 100, "slider to set");
            compare(slider.stepSize, 1, "slider step size set");
            compare(item.labelText, "24 %", "slider label set with suffix");

            // rejecting dialog
            item.clicked();
            verify(dialog.visible, "dialog to edit range value manually is open");
            compare(dialog.title, "Range key", "dialog title set");
            compare(dialog.text, "24", "dialog text set");
            dialog.text = "50";
            dialog.reject();
            compare(slider.value, 24, "slider value not updated");
            compare(root.config.rangeKey, 24, "new range value not applied after dialog was rejected");

            // accepting dialog
            item.clicked();
            compare(dialog.text, "24", "dialog text reset");
            dialog.text = "50";
            dialog.accept();
            compare(slider.value, 50, "slider value updated");
            compare(root.config.rangeKey, 50, "new range value applied after dialog was accepted");

            // use slider
            slider.increase();
            compare(root.config.rangeKey, 51, "new range value applied via slider");
        }

        function test_object_type() {
            const item = getDelegateForKey("objectKey");

            compare(myStackView.depth, 1, "stack has 1 item");

            item.clicked();
            tryCompare(myStackView, "depth", 2);

            const currentItem = myStackView.currentItem;
            verify(currentItem !== null, "current item exists");
            compare(currentItem.title, "Object Key", "new page title is correct");

            myStackView.pop();
            tryCompare(myStackView, "depth", 1);
        }

        function test_boolean_type() {
            const item = getDelegateForKey("booleanKey");

            const sw = findChild(item, (c) => c !== item && c.checked !== undefined);
            verify(sw !== null, "Switch found");
            compare(sw.checked, false, "initially false");

            const index = objectConfigPage.indexByKey["booleanKey"];
            objectConfigPage.updateValue(index, "booleanKey", true);
            compare(sw.checked, true, "now true");
            compare(root.config.booleanKey, true, "config booleanKey is true");

            objectConfigPage.updateValue(index, "booleanKey", false);
            compare(sw.checked, false, "back to false");
            compare(root.config.booleanKey, false, "config booleanKey is false");
        }

        function test_function_type() {
            const item = getDelegateForKey("functionKey");
            root.functionCalled = false;

            item.clicked();
            verify(root.functionCalled, "function clicked and invoked");
        }

        function test_filepath_type() {
            const item = getDelegateForKey("filepathKey");
            const manualBtn = item.manualButton;
            const dialog = item.dialog;

            manualBtn.clicked();
            verify(dialog.visible, "manual file dialog is open");
            compare(dialog.text, "/path/to/file", "initial text is set");

            dialog.text = "/path/to/another/file";
            dialog.reject();
            compare(root.config.filepathKey, "/path/to/file", "filepath not updated on reject");

            manualBtn.clicked();
            dialog.text = "/path/to/another/file";
            dialog.accept();
            compare(root.config.filepathKey, "/path/to/another/file", "filepath updated on accept");
        }

        function test_folderpath_type() {
            const item = getDelegateForKey("folderpathKey");
            const manualBtn = item.manualButton;
            const dialog = item.dialog;

            manualBtn.clicked();
            verify(dialog.visible, "manual folder dialog is open");
            compare(dialog.text, "/path/to/folder", "initial text is set");

            dialog.text = "/path/to/another/folder";
            dialog.reject();
            compare(root.config.folderpathKey, "/path/to/folder", "folderpath not updated on reject");

            manualBtn.clicked();
            dialog.text = "/path/to/another/folder";
            dialog.accept();
            compare(root.config.folderpathKey, "/path/to/another/folder", "folderpath updated on accept");
        }
    }
}
