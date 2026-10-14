# JRPG workspace

The JRPG workspace is the "JRPG" tab next to 2D, 3D, Script, Game and Asset Store. It is where Talewright's
JRPG tools live: the database, maps, events and battle setup.

## Turning it on and off
The tab appears only in projects that use it, so ordinary Godot projects look exactly as before.

- **Turn it on:** Project ▸ Project Settings ▸ General ▸ `jrpg/enabled`. The tab appears right away; no restart is
  needed.
- **Turn it off:** use **Turn off JRPG workspace** in the tab, or clear the same setting. Your files are not touched;
  only the tab goes away.

`jrpg/enabled` is the only JRPG project setting. Everything else a JRPG project needs (title, start position,
battle rules, …) lives in the project's System data inside the JRPG database.

## Setting up a project
**Set up JRPG project** creates the JRPG database, a first map and the JRPG input actions, and leaves your existing
scenes, scripts and settings as they are. This tool arrives with task 6.2 of the Talewright plan; until then the
button is shown but disabled.

## For tools and Claude Code
The workspace can be switched from outside the editor through the Claude bridge
(`talewright/docs/claude_bridge.md`):

```
python talewright/tools/bridge.py --project <dir> call project_settings_set name=jrpg/enabled value=true
python talewright/tools/bridge.py --project <dir> call editor_set_main_screen name=JRPG
```
