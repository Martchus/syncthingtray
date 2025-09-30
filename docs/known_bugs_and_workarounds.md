# Known bugs and workarounds
The following bugs are caused by dependencies or limitations of certain
platforms. For bugs of Syncthing Tray itself, checkout the issues on GitHub.

## Workaround positioning issues under Wayland
The Qt Widgets based version basically works under Wayland but there are
positioning issues and the settings regarding positioning have no effect (see
"[List of bugs](#list-of-bugs)" section below). One can workaround this limitation by telling the
window manager how to place the window, e.g. under Sway one could add a
configuration like this:

```
for_window [title="^Syncthing Tray( \(.*\))?$"] floating enable, border none, resize set 450 400, move position 916 0
```

Alternatively, one can also configure Syncthing Tray to use a normal window in
the appearance settings. That doesn't fix the positioning issue but then it
looks just like a normal application so not being positioned in the tray area is
less problematic.

You can also select the window type "None". This disables Syncthing Tray's own UI
completely and instead opens Syncthing directly when the tray icon is clicked.

## Tweak GUI settings for dark mode under Windows
The dark mode introduced in Windows 10 does not affect traditional desktop
applications like Syncthing Tray. As of version 6.7 the underlying toolkit Qt
nevertheless provides a style specifically for Windows 11 that supports dark mode.
So as of Qt 6.7 the dark mode should work out of the box on Windows 11. Otherwise
you can select the widgets style "Fusion" under "Qt/Appearance". Then Syncthing
Tray will no longer use native styling of traditional desktop apps and follow the
dark mode setting (as
[Qt 6.5 added dark mode support](https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5)).

It is also recommended to apply some further tweaks:

* Ensure an icon theme that looks good on dark backgrounds is selected. The Windows
  builds provided on GitHub bundle a version of Breeze for light and dark themes. By
  default the version matching the current color palette is selected automatically.
  If you had an icon theme configured explicitly, you may need to manually select a
  different icon theme in the settings under "Qt/Appearance" when enabling dark mode.
* To make Syncthing icons fit better with the dark color palette, configure their
  colors in Syncthing Tray's settings under "Tray/UI icons" and "Tray/System
  icons". The "Use preset" button allows to select pre-defined colors suitable for
  a dark color palette.

When using an older Qt version than 6.5 you will also have to resort to more manual
tweaking:

* To enable dark colors for Syncthing Tray's UI elements, configure a dark color
  palette in Syncthing Tray's settings under "Qt/Appearance". You can download and
  load [dark-palette.ini](https://raw.githubusercontent.com/Martchus/syncthingtray/master/tray/resources/dark-palette.ini)
  as a base and tweak the colors to your liking.
* As of Qt 6.4, dark window borders will be enabled automatically if Windows'
  dark mode setting is enabled and a dark color palette has been selected as
  mentioned in the previous step.
  To enable dark window borders in earlier Qt versions, set the environment
  variable `QT_QPA_PLATFORM` to `windows:darkmode=1` or create a file called
  `qt.conf` next to `syncthingtray.exe` with the contents:
  ```
  [Platforms]
  WindowsArguments = darkmode=1
  ```

When using Syncthing Tray 1.3.x or older, you need to restart Syncthing Tray for
these changes to have any effect. It is not sufficient to close the last window;
the process needs to be restarted.

Note that one can alternatively also enable Windows' "High contrast" setting which
seems to bring back the traditional theming/coloring (which has normally been
[removed](https://superuser.com/questions/949920/window-color-and-appearance-removed-in-win10)).
Unfortunately it doesn't look very nice overall. Checkout
https://github.com/tomasz1986/classic2000 to see how Windows looks like with high
contrast applied, or if you're in need for themes that look at least nicer than
what's shipped with Windows.

## DPI awareness under Windows
Syncthing Tray supports
[PMv2](https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows#per-monitor-and-per-monitor-v2-dpi-awareness)
out of the box as of Qt 6. You may tweak settings according to the
[Qt documentation](https://doc.qt.io/qt-6/highdpi.html#configuring-windows).

## Workaround broken High-DPI scaling of Plasmoid under X11
This problem [has been resolved](https://bugs.kde.org/show_bug.cgi?id=356446#c88) so
make sure you are using an up-to-date Plasma version. Otherwise, setting the environment
variable `PLASMA_USE_QT_SCALING=1` might help.

## List of bugs
* Wayland limitations
    * The tray menu can not be positioned correctly under Wayland because the protocol does not allow setting window positions from
      the client-side (at least I don't know a way to do it). This issue can not be fixed unless Wayland provides an API to set the
      window position to specific coordinates or a system tray icon (see discussion on
      [freedesktop.org](https://lists.freedesktop.org/archives/wayland-devel/2014-August/017584.html).
      The Plasmoid is *not* affected by this limitation.
    * While the tray menu is shown its entry is shown in the taskbar. Not sure whether there is a way to avoid this.
* Qt limitations and bugs
    * any Qt version:
        * The notification text is not translated under Android due to https://bugreports.qt.io/browse/QTBUG-140482.
        * Reading the initial status of the network connection requires manual workarounds on Android, see
          https://bugreports.qt.io/browse/QTBUG-139604.
    * Qt < 6.11:
        * Edge-to-edge is disabled under Android due to https://bugreports.qt.io/browse/QTBUG-139690.
    * Qt < 6.9.3:
        * Flickering might occur under Android, see https://bugreports.qt.io/browse/QTBUG-132718.
    * Qt = 6.9.2:
        * Animations don't work on many platforms, see https://bugreports.qt.io/browse/QTBUG-139630.
    * Qt < 6.7:
        * The native style does not look good under Windows 11. Therefore the style "Fusion" is used instead by default.
    * Qt < 6.5:
        * The dark mode introduced in Windows 10 is not supported, see https://bugreports.qt.io/browse/QTBUG-72028.
    * Qt < 6:
        * The tray disconnects from the local instance when the network connection goes down. The network connection must be restored
          or the tray restarted to be able to connect to local Syncthing again. This is caused by Qt bug
          https://bugreports.qt.io/browse/QTBUG-60949.
    * Qt < 5.14
        * Any self-signed certificate is accepted when using Qt WebEngine due to https://bugreports.qt.io/browse/QTBUG-51176.
    * Qt < 5.9:
        * Pausing/resuming folders and devices doesn't work when using scan-intervals with a lot of zeros because of
          Syncthing bug https://github.com/syncthing/syncthing/issues/4001. This has already been fixed on the Qt-side with
          https://codereview.qt-project.org/#/c/187069/. However, the fix is only available in Qt 5.9 and above.
        * Redirections cannot be followed (e.g. from HTTP to HTTPS) because
          `QNetworkRequest::RedirectPolicyAttribute` and `QNetworkRequest::NoLessSafeRedirectPolicy` are not available yet.
* KDE limitations
    * Issues with the configuration have appeared as of some version of Plasma, see https://github.com/Martchus/syncthingtray/issues/239
      and https://bugs.kde.org/show_bug.cgi?id=485072. It is unclear whether the problem is in Syncthing Tray or Plasma. Checkout the
      "[Configuring Plasmoid](../README.md#required-system-configuration)" section for details.
    * High-DPI scaling of Plasmoid is broken under X11 (https://bugs.kde.org/show_bug.cgi?id=356446).
    * Plasma < 5.26.0:
        * The Plasmoid contents are possibly clipped when shown within the system notifications plasmoid.
* Systemd integration
    * This feature relies especially on the system being correctly configured. Checkout the
      "[Required system configuration](../README.md#required-system-configuration)" section for details.
