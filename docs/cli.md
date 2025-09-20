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
    * The experimental mobile UI can be launched on the desktop with the `qt-quick-gui` sub-command
      when Syncthing Tray was built with support for it.

`syncthingtray` and `syncthingctl` support Bash completion when installed via GNU/Linux packaging.
In case of `syncthingctl` the completion also works for folder and device names/IDs.

## Important remarks
On Windows, you'll have to use the `syncthingtray-cli` executable to see output in the terminal.
(At least when using the official terminals; with e.g. mintty you can just use the normal executable.)

Depending on how you [downloaded/installed](https://martchus.github.io/syncthingtray/#downloads-section)
Syncthing Tray you may need get `syncthingctl` separately, e.g. as a separate download/package.

The exact name of the executables may also differ, e.g. the Arch Linux package uses `syncthingctl-qt6`
and `syncthingtray-qt6` for the preferable Qt 6 based version.

## Examples
The following sections give examples of some useful commands. For an exhaustive list of commands, use
`--help` as mentioned before.

`syncthingtray` only supports the `cli` sub-command for accessing Syncthing's own CLI when Syncthing is
built-in. Otherwise you can use `syncthing` instead of `syncthingtray` in these examples.

These examples assume that Bash is used. Other shells might require different quoting/escaping.

### Displaying status
One of the most useful commands is `status`:
```
syncthingctl status
```

It prints an accumulation of the information returned by several routes of the
[REST-API](https://docs.syncthing.net/dev/rest.html) and is comparable with the information displayed
on the official web-based UI.

It is also possible to filter the output:
```
syncthingctl status --dir dir1 --dir dir2 --dev dev1 --dev dev2
```

### Triggering actions
The `syncthingctl` commands `pause`, `resume` and `rescan` are also very useful as they allow quickly
pausing/resuming and rescanning the specified folders/devices from the command-line. The helptext
of `syncthingctl` already contains some examples.

### Querying and changing the configuration
You can also query and change the Syncthing configuration. Depending on the use case either
`syncthingctl` or Syncthing's own CLI client is better suited.

---

This `syncthingctl` command outputs the data
returned by the [configuration REST-API](https://docs.syncthing.net/rest/config.html#rest-config)
as raw JSON:

```
syncthingctl cat
```

You can use e.g. `jq` to get e.g. a specific field of a specific folder:

```
syncthingctl cat | jq -r '.folders[] | select(.id == "docs-books-recent") | .path'
```

The same can be achieved using Syncthing's own CLI client:

```
syncthingtray cli config folders docs-books-recent path get
syncthingtray cli config folders docs-books-recent dump-json | jq -r .path
```

---

The command `syncthingctl edit` allows editing the JSON config in a text editor on the
terminal (using the editor from your `EDITOR` environment variable). This corresponds to
changing the "Advanced Configuration" on the official web-based UI so, **be careful!**
Incorrect configuration may damage your folder contents and render Syncthing inoperable.

It is also possible to change the config programmatically via JavaScript, e.g. this
example toggles whether a specific folder is paused:
```
syncthingctl edit --js-lines \
    'config.folders.filter(f => f.id === "docs-misc").forEach(f => f.paused = !f.paused)'
```

You can use the usual JavaScript APIs, e.g.
[`startsWith()`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/startsWith)
to pause several folders at once:
```
syncthingctl edit --js-lines \
    'config.folders.filter(f => f.id.startsWith("docs-")).forEach(f => f.paused = !f.paused)'
```

You probably want to test such a commands before executing them for real with `--dry-run`
first.

Syncthing's own CLI allows something similar without involving JavaScript:

```
syncthingtray cli config folders docs-misc paused set true
```
