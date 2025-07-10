---
name: Bug report
about: Report an existing feature not working as designated
title: ''
labels: bug
assignees: ''

---

Please read the following points before filing an issue but remove them before
filing the issue:

* **Do not file an issue if the executable is framed as malicious by Antivirus
  software.** If you have actual evidence that the executable is doing anything
  malicious you can of course file a report. Note that the executable being
  detected by Antivirus software is not good enough evidence as false positives
  are very common and no concrete description of the problem is provided.
* **Before reporting, please have a look at "[Known bugs and workarounds](https://github.com/Martchus/syncthingtray/blob/master/docs/known_bugs_and_workarounds.md)".**
* Note that I cannot support all operating systems, their flavors and different
  tooling you might be using (Antivirus scanners, GNU/Linux desktop environments,
  AUR helpers, …). So please avoid filing bug reports specific to them and contact
  the respective vendors instead.
* Note that adaptions for newer versions of certain platforms (or for completely
  new platforms) would be *feature requests* and **not** bugs. So for instance,
  making Syncthing Tray work under an updated/new GNU/Linux desktop environment
  should be filed as a feature request and *not* a bug report.
* Note that I will likely have to reject bug reports about Wayland-specific
  problems due to limitations of that protocol which I cannot workaround from my
  side.

---

**Relevant components**
* [ ] Standalone tray application (based on Qt Widgets)
* [ ] Plasmoid/applet for Plasma desktop
* [ ] Dolphin integration
* [ ] Command line tool (`syncthingctl`)
* [ ] Integrated Syncthing instance (`libsyncthing`)
* [ ] Android app or mobile UI in general
* [ ] Backend libraries

**Environment and versions**
* Versions of `syncthingtray`, `qtutilities` and `c++utilities`: …, …, …
* Qt version: ….….…
* C++ compiler (name and version): …
* C++ standard library (name and version): …
* Operating system (name and version): …

**Bug description**
A clear and concise description of what the bug is.

**Steps to reproduce**
1. …
2. …

**Expected behavior**
A clear and concise description of what you expected to happen.

**Screenshots**
If applicable, add screenshots to help explain your problem.

**Additional context**
Add any other context about the problem here.
