# Talewright managed addon

This folder holds the scripts and scenes that Talewright's JRPG tools build games with: screens, menus and the
pieces later epics add. The JRPG setup tool copies it into the project; the editor keeps it current.

## Editing these files
You can change any file here. The upgrade tool (Project > Tools > Set up JRPG Project, or the `jrpg_addon_install`
bridge tool) replaces a file only when it is exactly as Talewright installed it. Files you changed are kept and
listed in the upgrade report, so you can merge newer versions by hand.

## Contents
- `scenes/game.tscn`: the game, a `JRPGSceneManager` root. The JRPG workspace's Play From Start runs it: a new game at
  the start position set in System data.

`addon.cfg` records the installed version and the hash of every installed file. Do not edit it.
