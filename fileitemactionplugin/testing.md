# Testing and debugging Dolphin/KIO plugin with Qt Creator
1. Build as usual, ensure `NO_FILE_ITEM_ACTION_PLUGIN` is turned off.
2. Copy `*.desktop` file to `~/.local/share/kservices5` and enable it in Dolphin if a packaged version
   with the same configuration name is not already installed. Replace `5` with the current major version
   of KDE.
3. Add new config for run in Qt Creator and set `dolphin` as executable.
4. In execution environment, set
  * `QT_PLUGIN_PATH` to directory containing plugin `\*.so`-file.
  * `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection.
  * `QT_XCB_NO_GRAB_SERVER` to 1 to prevent grabbing so the debugger is usable.
5. Ignore warning that executable is no debug build, it is sufficiant when plugin is debug build.

## Testing against a development build of Dolphin
NOTE: This does not actually work with a KF6 build. Maybe the `*.desktop` file needs to be changed.

1. Build the whole dependency chain up to `dolphin` installing it under some custom prefix.
2. Then follow the usual steps but make sure you build Syncthing Tray against the custom KDE builds.
   This is achieved the easiest by using the `debug-kde` CMake preset. This preset uses the environment
   variable `KDE_INSTALL_DIR` which must point to the custom prefix used in step 1.
3. Copy the `*.desktop` file into the custom prefix used in step 1 under `share/kservices6`. Replace `6`
   with the current major version of KDE. The directory likely needs to be created first.
4. Source the `prefix.sh` script that should be present in the build directory of any KDE library
   you built in step 1, e.g. `source kde/plasma-sdk/prefix.sh`.
5. When setting the environment one needs to be more careful to not override variables set in step 4.
   It is the easiest to just start e.g. `plasmawindowed` from the shell:
   ```
   QT_PLUGIN_PATH=$BUILD_DIR/syncthingtray/debug-kde/syncthingtray/fileitemactionplugin:$QT_PLUGIN_PATH dolphin
   ```
