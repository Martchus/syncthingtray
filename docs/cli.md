# Using the command-line interface
Syncthing Tray provides two command-line interfaces:

* The separate executable `syncthingctl` allows to interact with a running instance of Syncthing to
  trigger certain actions like rescans, editing the Syncthing config and more. It complements
  Syncthing's own command-line interface. Invoke `syncthingctl --help` for details.
* The GUI/tray executable `syncthingtray` also exposes a command-line interface to interact with
  a running instance of the GUI/tray. Invoke `syncthingtray --help` for details. Additional remarks:
    * If Syncthing itself is built into Syncthing Tray (like the Linux and Windows builds found in
      the release-section on GitHub) then Syncthing's own command-line interface is exposed via
      `syncthingtray` as well.
    * On Windows, you'll have to use the `syncthingtray-cli` executable to see output in the terminal.
    * The experimental mobile UI can be launched on the desktop with the `qt-quick-gui` sub-command
      when Syncthing Tray was built with support for it.
