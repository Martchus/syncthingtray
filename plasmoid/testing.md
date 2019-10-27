# Testing and debugging Plasma 5 plasmoid with Qt Creator

The following instructions allow to test the Plasmoid by installing it in a test directory
rather than the regular home to separate testing from production.

1. Build as usual, ensure `NO_PLASMOID` is turned off
2. Add build step to execute custom target `init_plasmoid_testing` which
   will install the Plasmoid in a test directory which is "$BUILD_DIR/plasmoid/testdir"
   by default
3. Add new config for run in Qt Creator and set `plasmoidviewer` (or `plasmawindowed`)
   as executable
4. In execution environment, set
  * `QT_PLUGIN_PATH` to `$BUILD_DIR/plasmoid/lib` which should be containing the plugin
    for the Plasmoid under `plasma/applets/libsyncthingplasmoid.so`
  * `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection
  * `HOME` to the test directory from step 2 so plasmoidviewer finds the Plasmoid
    in the test directory
5. Set `--applet martchus.syncthingplasmoid` as CLI argument
6. Ignore warning that executable is no debug build, it is sufficiant when
   the plugin is a debug build (see next section for QML debugging)

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
