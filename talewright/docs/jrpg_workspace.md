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
**Set up JRPG project** (the button in the empty tab, or Project ▸ Tools ▸ Set Up JRPG Project...) creates what is
missing and keeps everything that exists: the `res://jrpg` folders, the JRPG database with a few starter entries
(`res://jrpg/database.tres` and one file per table under `res://jrpg/db`), the `jrpg_*` input actions, the
`jrpg/enabled` setting, a game id for save folders and the Talewright addon (`res://addons/talewright`). Your scenes,
scripts and other settings stay as they are. Running it again only adds what is missing; a file that holds the wrong
kind of data is reported instead of being replaced. Maps arrive with the map tools.

## The Database section
The **Database** tab lists the tables on the left, the selected table's entries in the middle (`0001  Hero`: an
entry's ID is its slot and never moves) and the selected entry's form on the right.

- **Change Maximum...** adds empty entries at the end or removes the last ones; when removed entries are still used
  elsewhere, the dialog lists those references first.
- **Clear** empties an entry (its ID stays), listing what still points at it first; **Copy** and **Paste** copy a
  whole entry within the same table.
- ID fields are searchable pickers that list the entries when opened; traits, effects and other lists are edited
  through their own dialogs; an enemy action's values follow its condition (turns, an HP or MP band, a state...).
- Every edit can be undone with the editor's Undo. Changed tables show `(*)`; **Save** writes them, and so do the
  editor's own saves (saving a scene, running the project) and the prompt before quitting or reloading.

### Tilesets
A tileset's form shows its **TileSet** (a Godot TileSet file; **Create TileSet** makes one of 48 x 48 pixel tiles in
`res://jrpg/tilesets/`) and its **Tiles**: every tile maps can use, an autotile or wall kind shown once.

- **Add Sheet...** brings in an image: plain tiles (each tile that shows anything), an autotile or a wall laid out
  as in `talewright/docs/autotile_template.md`. The dialog guesses the kind from the image size and tells which size
  each kind needs.
- The buttons above the tiles choose which flag is drawn over them: Passage (ring open, cross blocked, broken ring
  transparent), Entry Sides and Exit Sides (red bars on blocked sides), Overhead, Ladder, Bush, Counter, Damage Floor
  and Terrain Tag. A click changes the clicked tile (right-click cycles back; in the side modes, the edge clicked);
  Enter does the same for the selected tile and the side check boxes set single sides from the keyboard.
- An autotile or wall kind changes on all its shapes at once. Each click is one Undo step.
- The flags live in the TileSet's `jrpg_*` custom data layers, so Godot's TileSet editor shows and paints them too;
  saving the database bakes them into the tileset entry for maps and servers. Flags painted in the TileSet editor
  show up in the Database section's unsaved list and are baked by any save (the section's Save, saving a scene,
  running the project).
- A yellow dot on an autotile or wall marks shapes painted with different flags; the first shape's flags are shown
  and used, and a click sets the new value on every shape. A flipped or rotated tile shares its flags with the tile
  it turns.

## For tools and Claude Code
The workspace can be switched from outside the editor through the Claude bridge
(`talewright/docs/claude_bridge.md`):

```
python talewright/tools/bridge.py --project <dir> call project_settings_set name=jrpg/enabled value=true
python talewright/tools/bridge.py --project <dir> call editor_set_main_screen name=JRPG
```
