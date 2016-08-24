# Syncthing Tray

Qt 5-based tray application for [Syncthing](https://github.com/syncthing/syncthing)

* Still under development
* Designed to work under any desktop environment with tray icon support
* Doesn't require desktop environment specific libraries
* Provides quick access to most frequently used features but does not intend to replace the official web UI
* Shows Syncthing notifications
* Provides quick access to the official web UI
  * Utilizes either Qt WebKit or Qt WebEngine
  * Can be built without web view support as well (then the web UI is opened in the regular browser)

## Screenshots
### Openbox/Tint2
![Openbox/Tint2](/resources/screenshots/1.png?raw=true "Under Openbox with Tint2")

## Download / binary repository
I will provide packages for Arch Linux and Windows when releasing. For more information checkout my
[website](http://martchus.no-ip.biz/website/page.php?name=programming).

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and [qtutilities](https://github.com/Martchus/qtutilities) and is built the same way as these libaries. For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).

The following Qt 5 modules are requried: core network gui network widgets webenginewidgets/webkitwidgets

#### Select Qt modules for WebView
* If Qt WebKitWidgets is installed on the system, the tray will link against it. Otherwise it will link against Qt WebEngineWidgets.
* To force usage of Qt WebKit/Qt WebEngine or to disable both add `-DWEBVIEW_PROVIDER=webkit/webengine/none` to the CMake arguments.
