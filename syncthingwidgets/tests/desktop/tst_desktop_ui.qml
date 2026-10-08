import QtQuick
import QtQuick.Controls.Material
import QtTest
import Main
import Tray

Item {
    id: root
    width: 640
    height: 480

    TrayView {
        id: trayView
        anchors.fill: parent
        readonly property DesktopWindow window: DesktopWindow {
            id: desktopWindow
        }
        readonly property PageWindow pageWindow: PageWindow {
            id: pageWindow
            page: Page {
                id: page
                title: "test"
            }
        }
    }

    TestCase {
        id: testCase
        name: "DesktopUITests"

        function initTestCase() {
        }

        function findChildByTypeName(parentObj, typeName) {
            if (!parentObj || !parentObj.children) return null;
            for (let i = 0; i < parentObj.children.length; i++) {
                let child = parentObj.children[i];
                if (child.toString().indexOf(typeName) !== -1) {
                    return child;
                }
                let found = findChildByTypeName(child, typeName);
                if (found) {
                    return found;
                }
            }
            return null;
        }

        function test_tabSwitching() {
            const stackLayout = findChildByTypeName(trayView, "StackLayout");
            const tabBar = findChildByTypeName(trayView, "TabBar");

            verify(stackLayout !== null, "StackLayout found");
            verify(tabBar !== null, "TabBar found");

            // verify initial state
            compare(tabBar.currentIndex, 0, "folders tab active by default");
            compare(stackLayout.currentIndex, 0, "folders list shown by default");

            // click the second tab (devices)
            tabBar.itemAt(1).click();
            compare(tabBar.currentIndex, 1, "tab bar active index updated");
            compare(stackLayout.currentIndex, 1, "devices list shown");

            // click the third tab (recent changes)
            tabBar.itemAt(2).click();
            compare(tabBar.currentIndex, 2, "tab bar active index updated");
            compare(stackLayout.currentIndex, 2, "recent changes shown");
        }

        function test_desktopWindow() {
            verify(desktopWindow.header !== null, "header present");
            verify(desktopWindow.theming !== null, "theming present");
            verify(desktopWindow.meta !== null, "meta present");
        }

        function test_pageWindow() {
            verify(pageWindow.currentPage === page, "page shown");
            verify(pageWindow.theming !== null, "theming present");
            verify(pageWindow.meta !== null, "meta present");
        }
    }
}
