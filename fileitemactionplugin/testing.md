# Testing and debugging Dolphin/KIO plugin with Qt Creator

1. Build as usual, ensure `NO_FILE_ITEM_ACTION_PLUGIN` is turned off
2. Copy `*.desktop` file to `~/.local/share/kservices5` and enable it in Dolphin
   if packaged version is not already installed
3. Add new config for run in Qt Creator and set `dolphin` as executable
4. In execution environment, set
  * `QT_PLUGIN_PATH` to directory containing plugin `\*.so`-file
  * `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection
  * `QT_XCB_NO_GRAB_SERVER` to 1 to prevent grabbing so the debugger is usable
5. Ignore warning that executable is no debug build, it is sufficiant when
   plugin is debug build
