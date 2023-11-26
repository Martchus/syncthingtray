# Testing and debugging Plasmoid for Plasma with Qt Creator
The following instructions allow to test the Plasmoid by installing it in a test directory
rather than the regular home to separate testing from production.

1. Build as usual, ensure `NO_PLASMOID` is turned off.
2. Add build step to execute the custom target `init_plasmoid_testing` which
   will install the Plasmoid in a test directory which is `$CMAKE_BUILD_DIR/plasmoid-testing`
   by default (configurable via cache variable `PLASMOID_TESTDIR`, the sub directory
   `plasmoid-testing` is not part of the variable).
3. Add new config for run in Qt Creator and set `bash` as executable.
4. Set `%{sourceDir}/../../syncthingtray/plasmoid/scripts/starttesting.sh plasmoidviewer --applet martchus.syncthingplasmoid`
   as CLI argument.
  * It is also possible to use `plasmawindowed` or `plasmashell`, see sections below.
  * It is also possible to specify `org.kde.plasma.systemtray` as applet to test how the Plasmoid
    looks like within the system tray plasmoid.
  * This usage of `%{sourceDir}` assumes one used the "Building this straight" instructions
    from the main README.md.
  * When using a suffix (e.g. development build via CMake presets), this suffix needs to be appended
    to the applet name.
5. Keep `%{buildDir}` as working directory.
6. In execution environment there's nothing mandatory to be set because `starttesting.sh` should
   already take care of setting the environment.
    * The home directory is set in accordance with the directory used in step 2. but can be overridden
      by setting `TEST_HOME`; make sure that `TEST_HOME` and the CMake variable `PLASMOID_TESTDIR` are
      set in accordance.
    * If not already set, `QT_PLUGIN_PATH` is set to `$CMAKE_CURRENT_BINARY_DIR/plasmoid/lib` which
      should contain the plugin for the Plasmoid under `plasma/applets/*syncthingplasmoid*.so`.
    * Set `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection (the verbose output might be suppressed
      when starting via Qt Creator so it may be worthwile to start this from a terminal).
7. Ignore warning that executable is no debug build, it is sufficient when
   the plugin is a debug build (see next section for QML debugging).

## Saving/restoring settings

Be aware that `plasmoidviewer` will revert Plasmoid-specific settings to the defaults on
startup. So it is not possible to test restoring/saving settings using it.
For this use case, `plasmawindowed` can be used instead.

## Testing within the real Plasma shell

Some issues are only reproducible within the actual Plasma shell. It is possible to test
with the real Plasma shell in the same way as described above by setting `plasmashell` as
executable.

It is only possible to run one `plasmashell` at a time so you have to stop your regular
`plasmashell` first. While developing you can start e.g. `tint2` to be not without a shell.
It works quite well within a Plasma session when both shells are placed on different screen
edges.

## Enable QML debugging
It is not clear whether the following instructions are still valid for Plasma 6.
It seems that QML debugging can be enabled under Plasma 6 by setting the environment variable
`PLASMA_ENABLE_QML_DEBUG` but this has not been tested yet.

To enable QML debugging, it is required to rebuild `plasmoidviewer` with QML debugging
enabled.

For Arch Linux, I created the package
[`plasmoidviewer-debug`](https://github.com/Martchus/PKGBUILDs/tree/master/plasmoidviewer-debug/default)
for that purpose. Installing this package and using `plasmoidviewer-debug` instead of `plasmoidviewer`
should make enabling QML debugging in the *Run* section of Qt Creator work.

To create a debug build of `plasmoidviewer` manually:

1. Get plasma-sdk: `git clone https://anongit.kde.org/plasma-sdk.git`
2. Create a debug build of `plasmoidviewer` and ensure `QT_QML_DEBUG` is defined when
   compiling `plasmoidviewer`, eg. by applying
   [[PATCH] Enable QML debugging](https://raw.githubusercontent.com/Martchus/PKGBUILDs/master/plasmoidviewer-debug/default/0001-Enable-QML-debugging.patch).
3. Prepend the build directory containing the `plasmoidviewer` binary to the path variable
   in the build environment of Syncthing Tray.
4. Enable QML debugging in the *Run* section.

# Testing against a development build of Plasma
1. Build the whole dependency chain up to `plasma-desktop` installing it under some custom prefix.
   Note that `plasma-sdk` alone is not sufficient.
2. Then follow the usual steps but make sure you build Syncthing Tray against the custom KDE builds.
   This is achieved the easiest by using the `debug-kde` CMake preset. This preset uses the environment
   variable `KDE_INSTALL_DIR` which must point to the custom prefix used in step 1.
3. Source the `prefix.sh` script that should be present in the build directory of any KDE library
   you built in step 1, e.g. `source kde/plasma-sdk/prefix.sh`.
4. When setting the environment one needs to be more careful to not override variables set in step 3.
   It is the easiest to just start e.g. `plasmawindowed` from the shell:
   ```
   QT_PLUGIN_PATH=$BUILD_DIR/syncthingtray/debug-kde/syncthingtray/plasmoid/lib:$QT_PLUGIN_PATH HOME=$BUILD_DIR/syncthingtray/debug-kde/plasmoid-testing kdeinstall/bin/plasmawindowed martchus.syncthingplasmoid-devel
   ```
      * It would make sense to tweak `starttesting.sh` to be able to do the sourcing automatically.
