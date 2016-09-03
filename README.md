# Syncthing Tray

Qt 5-based tray application for [Syncthing](https://github.com/syncthing/syncthing)

* Designed to work under any desktop environment with tray icon support
  * Tested under
    * Plasma 5
    * Openbox/qt5ct/Tint2
    * Cinnamon
  * Could be shown as regular window if no tray icon support is available
* Doesn't require desktop environment specific libraries
* Provides quick access to most frequently used features but does not intend to replace the official web UI
  * Check state of directories and devices
  * Check current traffic statistics
  * Display further details about direcoties and devices, like last file, last
    scan, ...
  * Trigger re-scan of a specific directory or all directories at once
  * Open a directory with the default file browser
  * Pause/resume a specific device or all devices at once
* Shows Syncthing notifications
* Does *not* allow configuring Syncthing itself (currently I do not intend to add this feature as it could cause more harm than good when not implemented correctly)
* Can read the Syncthing configuration file for quick setup when just connecting to local instance
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing when started and display stdout/stderr (useful under Windows)
* Provides quick access to the official web UI
  * Utilizes either Qt WebKit or Qt WebEngine
  * Can be built without web view support as well (then the web UI is opened in the regular browser)
* Allows quickly switching between multiple Syncthing instances
* Still under development; the following features are planned
  * Show currently processed items
  * Show recently processed items
  * Improve notification handling

## Screenshots
### Under Openbox/Tint2
![Openbox/Tint2](/resources/screenshots/tint2.png?raw=true)

### Under Plasma 5 (dark color theme)
![Plasma 5](/resources/screenshots/plasma.png?raw=true)

### Settings dialog (dark color theme)
![Settings dialog](/resources/screenshots/settings.png?raw=true)

### Web view (dark color theme)
![Web view](/resources/screenshots/webview.png?raw=true)

## Download / binary repository
I will provide packages for Arch Linux and Windows when releasing. For more information checkout my
[website](http://martchus.no-ip.biz/website/page.php?name=programming).

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and [qtutilities](https://github.com/Martchus/qtutilities) and is built the same way as these libaries. For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).

The following Qt 5 modules are requried: core network gui widgets webenginewidgets/webkitwidgets

#### Select Qt modules for WebView
* If Qt WebKitWidgets is installed on the system, the tray will link against it. Otherwise it will link against Qt WebEngineWidgets.
* To force usage of Qt WebKit/Qt WebEngine or to disable both add `-DWEBVIEW_PROVIDER=webkit/webengine/none` to the CMake arguments.

BTW: I still prefer the deprecated Qt WebKit because
* I currently don't know how to allow a particular self-signed certificate in Qt WebEngine. (Currently any self-signed certificate is accepted!)
* Qt WebEngine can not be built with mingw-w64.
* Qt WebEngine is more buggy in my experience.
* security issues are not a concern because no other website than the Syncthing web UI is shown.
