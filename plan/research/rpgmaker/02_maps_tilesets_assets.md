# RPG Maker MZ feature inventory: maps, tilesets, assets

This is part of the Talewright clean-room reimplementation research. Every item below was checked against an official RPG Maker MZ help page fetched on 2026-10-13. All descriptions are paraphrased. Nothing is copied from the help text. Field, menu and command names are given as the editor labels them. Anything a page did not state is marked "not verified" here and listed again at the end.

Base URL: https://rpgmakerofficial.com/product/MZ_help-en/

## Pages covered

| Page | URL | Loaded? | Notes |
|---|---|---|---|
| Map | https://rpgmakerofficial.com/product/MZ_help-en/01_07.html | Yes | |
| Editing Map Designs | https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html | Yes | |
| Map Data Controls | https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html | Yes | |
| Map Properties | https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html | Yes | |
| Tileset Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html | Yes | |
| Asset Standards | https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html | Yes | |
| Side-view Character Standards (linked from Asset Standards) | https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html | Yes | Sub-link |
| How to Use Aid Tools (linked from Map Data Controls for Generate Dungeon) | https://rpgmakerofficial.com/product/MZ_help-en/01_05.html | Yes | Sub-link |
| Menu Bar Content (Mode, Layer, Scale and Edit menus for the map editor) | https://rpgmakerofficial.com/product/MZ_help-en/01_04.html | Yes | Supplementary: event mode, region mode, zoom |
| Basic Editor Controls (window components) | https://rpgmakerofficial.com/product/MZ_help-en/01_03.html | Yes | Supplementary |
| Database overview | https://rpgmakerofficial.com/product/MZ_help-en/01_08.html | Yes | Supplementary, no map content used |
| Animation Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html | Yes | Supplementary: animation asset kind |
| System 2 Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html | Yes | Supplementary: fonts |
| Using Official Plugins | https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html | Yes | Checked for asset info; none relevant used |
| Help top page | https://rpgmakerofficial.com/product/MZ_help-en/01.html | Yes | No table of contents in the fetched text |
| 01_11.html (guessed Asset section index) | https://rpgmakerofficial.com/product/MZ_help-en/01_11.html | **No (HTTP 404)** | Probably not a real page |

---

## MAP: map editor and map data controls

### RM-MAP-001 — Maps, tiles and tilesets (concept)
- **What:** A map is the setting the player walks around in. Its layout is built by placing square tiles, and the tiles come from the tileset assigned to the map.
- **Behavior:**
  - Each tile does two jobs. It supplies an image, and it carries settings such as whether characters can move onto it.
  - A tileset groups many tiles into one database item. Swapping a map's tileset changes the map's whole look at once.
  - Tilesets are edited in the Database (see RM-TIL-*).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07.html

### RM-MAP-002 — Tile classes and layering (lower vs upper)
- **What:** Tilesets hold five tile classes, A to E. A is the ground layer and B to E are the upper layers.
- **Behavior:**
  - Class A is the lower layer, used for terrain and ground. By default this covers sea, fields, floors and walls.
  - Classes B, C, D and E are upper layers, used for things on top of the ground such as trees, bushes and signs.
  - One map cell can hold a lower tile and an upper tile at the same time.
  - In MZ, two different upper-layer tiles can overlap in the same cell. See RM-MAP-019 for the stacking rules.
  - The help gives no explicit maximum for the total layer count per cell. The layer selector exposes four layers (RM-MAP-022).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07.html

### RM-MAP-003 — Map size and on-screen view
- **What:** Map dimensions are counted in tiles, and the camera follows the player on large maps.
- **Behavior:**
  - Width and height can each be set from 0 to 256 tiles.
  - The game screen shows 17 tiles across by 13 tiles down at one time. The help does not give the pixel size here.
  - On maps bigger than the view, the visible area automatically centers on the player. The help calls this scroll processing.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07.html

### RM-MAP-004 — Map looping (concept)
- **What:** A map's opposite edges can be joined so that it wraps around like a globe.
- **Behavior:**
  - The help calls this loop processing.
  - It is configured per map with the Scroll Type field (RM-MPR-006).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07.html

### RM-MAP-005 — Map coordinate system
- **What:** Tiles are addressed with X/Y coordinates.
- **Behavior:**
  - The origin (0,0) is the top-left tile. X grows to the right and Y grows downward.
  - On a 100×100 map, the bottom-right tile is (99,99).
  - The editor status bar shows the coordinates of the tile under edit.
  - Event commands use coordinates, for example to check where the party is or to send the party to a location taken from variables.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-006 — Map (design) editing mode
- **What:** This editor mode is for painting tiles onto a map.
- **Behavior:**
  - Enter it with the Map toolbar button or with Mode → Map in the menu.
  - The drawing tools (RM-MAP-010 to RM-MAP-016) are available in this mode.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-MAP-007 — Event editing mode
- **What:** This editor mode is for placing and editing map events, as opposed to painting tiles.
- **Behavior:**
  - Enter it with Mode → Event.
  - In this mode, the Map View draws grid lines matching the tile size.
  - Event editing itself is outside this slice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-MAP-008 — Map List and Map View panes
- **What:** These are the two main panes for choosing a map and editing it.
- **Behavior:**
  - The Map List sits at the bottom left and shows every map in the project. Choosing a map there opens its design in the Map View on the right.
  - The Map View is where tiles are painted and where event positions are chosen.
  - The status bar shows details of the selected function, the map name, or map coordinates.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-009 — Tile Palette and tile selection
- **What:** The palette shows the current map's tileset and is where you pick tiles to paint.
- **Behavior:**
  - Clicking a tile selects it, and a white frame marks the selection.
  - Dragging across the palette selects a rectangular block of tiles, which is then painted as a group.
  - Tabs A, B, C, D and E along the bottom switch which tile class the palette shows.
  - Tab A holds the autotiles plus the A2 base and decoration tiles.
  - The top-left cell of tab B is the transparent "empty" upper tile.
  - A separate tab at the bottom of the palette switches to region editing (RM-MAP-023).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-010 — Pencil tool
- **What:** Freehand drawing tool.
- **Behavior:**
  - Puts the selected tile (or tile block) wherever you click.
  - Dragging keeps painting.
  - Choose it from the toolbar or the Draw menu.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-011 — Rectangle tool
- **What:** Filled-rectangle drawing tool.
- **Behavior:**
  - Drag diagonally. The start and end points of the drag set opposite corners of a rectangle, and the rectangle is filled with the selected tile.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-012 — Ellipse tool
- **What:** Filled-ellipse drawing tool.
- **Behavior:**
  - Drag diagonally to define a bounding box. The selected tile fills the ellipse inscribed in that box.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-013 — Flood Fill tool
- **What:** Bucket-fill tool.
- **Behavior:**
  - Clicking a cell replaces that cell and every connected cell of the same tile with the selected tile.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-014 — Shadow Pen
- **What:** Paints or removes manual shadow darkening, mainly used next to walls.
- **Behavior:**
  - Choose it from the toolbar or from Draw → Shadow Pen, then click in the Map View.
  - It darkens in steps of one quarter of a tile.
  - It toggles. Clicking an unshadowed sub-cell darkens it, and clicking a shadowed sub-cell clears the shadow.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-015 — Erasing
- **What:** There is no separate eraser tool. You erase by painting a transparent tile.
- **Behavior:**
  - To clear upper layers, paint the transparent tile at the top-left of tab B.
  - That tile is the only one that wipes every upper tile in a cell (see RM-MAP-019).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-016 — Eyedropper (copy from map)
- **What:** Picks tiles up from the map so they can be painted elsewhere.
- **Behavior:**
  - Right-clicking a cell in the Map View picks up that cell's tile, and left-clicking then paints it.
  - Right-click-dragging picks up a rectangular range of cells for pasting.
  - Holding Shift while using the eyedropper turns autotile adjustment off for that use (see RM-MAP-017).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-017 — Autotile drawing
- **What:** Some tiles automatically pick border pieces that match their neighbors, so placing the same ground type joins up seamlessly.
- **Behavior:**
  - Autotiles are on tab A. Each autotile type holds several patterns, and the editor chooses the border variant from what is next to it.
  - Autotile images are the A1 to A4 slots in the tileset (RM-TIL-005, RM-TIL-016 to RM-TIL-028).
  - Holding Shift while drawing, or while using the eyedropper, temporarily disables autotiling, so a specific variant can be placed as-is.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-018 — Autoshadows
- **What:** A shadow is drawn automatically beside certain stacked autotiles.
- **Behavior:**
  - When two or more of certain autotiles (building and wall kinds) are stacked vertically, a shadow is added at the bottom right, on the neighboring tile.
  - Some tiles never get the shadow. Per the Asset Standards page, the shadow appears only when the right-hand neighbor is an A2 (ground) tile, with an unexplained "excluding Block C" exception, or an A5 tile. See RM-TIL-031.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MAP-019 — Upper-layer stacking rules
- **What:** Rules for what happens when upper tiles (B to E) are painted over each other.
- **Behavior:**
  - A cell keeps at most two different upper tiles.
  - Painting a third upper tile drops the oldest (first) one.
  - If the third tile is the same as the second (newer) one, the first is kept.
  - Only the top-left tile of tab B clears all upper tiles from a cell.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-020 — Passage resolution for stacked upper tiles
- **What:** Rules for working out whether a cell is passable when its tiles have different passage settings.
- **Behavior:**
  - If a passable tile and an impassable tile are stacked, the tile painted later decides passability.
  - A ☆ tile is still drawn above the character even when it is stacked over another passable ☆ tile.
  - When a ☆ (passable, drawn-over) tile is stacked with an impassable tile, the impassable setting wins for movement.
  - The exact display-order wording for two ☆ tiles was ambiguous in the fetched text. **Not verified** in detail.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-021 — A2 base and decoration rules (lower layer)
- **What:** Special lower-layer behavior for A2 ground tiles.
- **Behavior:**
  - The A2 sheet is split by column. The left half (columns 1 to 4) is "Base" and the right half (columns 5 to 8) is "Decoration".
  - Decoration tiles can be painted on top of a base tile in the same cell, so the lower layer can hold a base plus an overlay.
  - In Field Type tilesets only: if a decoration is painted over a base from column 2, the base becomes the column-1 tile. If it is painted over a base from column 4, the base becomes the column-3 tile.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-MAP-022 — Map layer selection (Auto / Layer 1–4)
- **What:** Restricts painting to one layer of the map.
- **Behavior:**
  - The options are Auto, Layer 1, Layer 2, Layer 3 and Layer 4. Each has an icon, and the Layer menu offers the same choice.
  - Auto picks the target layer for you as you draw. It is the normal mode.
  - Picking one numbered layer highlights that layer's tiles. Only that layer can be edited, and the others are untouched.
  - Typical use: after decorating with upper tiles, use Layer 1 to bulk-replace a room's floor without disturbing the decorations.
  - The help does not say which numbered layer corresponds to which tile class. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-023 — Region editing mode
- **What:** A mode for painting region IDs onto map cells. Regions mainly limit where random encounters happen.
- **Behavior:**
  - Enter it by choosing the [R] tab of the tileset palette. The Menu Bar page lists this under the Event entry of the Mode menu.
  - Choose a region ID from 1 to 255 in the top-left area of the palette, then click cells in the Map View to assign it.
  - A cell can carry only one region ID. A map can use up to 255 regions.
  - Encounter entries can be limited to regions (RM-MPR-020).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-024 — Map view zoom (Scale menu)
- **What:** Changes how large the Map View is drawn in the editor.
- **Behavior:**
  - Zoom In makes the view larger each time it is chosen.
  - Zoom Out makes the view smaller each time it is chosen.
  - Actual Size goes back to the standard 1:1 size.
  - The help does not list the specific zoom steps. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-MAP-025 — Undo and Edit-menu operations on maps
- **What:** Edit-menu operations that apply in the map editor.
- **Behavior:**
  - Undo reverts the last action, up to 20 actions back.
  - Cut, Copy, Paste and Delete work on map data and map events through the clipboard. Cut copies and then removes. Paste adds the clipboard contents as new map data or events.
  - Redo was not mentioned. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-MAP-026 — Editor map grid and transparent colour (Options)
- **What:** Editor display preferences that affect the map editor.
- **Behavior:**
  - The Map Grid "Show" checkbox turns a grid on in the map editor.
  - Transparent Color sets the background colour the editor uses to show transparent areas.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-MAP-027 — Playtest passability override
- **What:** A test-time shortcut for walking through blocked tiles.
- **Behavior:**
  - During a playtest, holding Ctrl while moving lets the player pass through impassable tiles.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-MAP-028 — Map List context menu
- **What:** Right-clicking a map in the Map List opens a menu of map-data operations.
- **Behavior:**
  - The commands are Edit, New, Load Sample Map, Copy, Paste, Delete, Shift, Generate Dungeon and Save as Image (RM-MAP-029 to RM-MAP-037).
  - The Basic Editor Controls page also describes a Search option on context menus, with Search Next and Search Previous.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-MAP-029 — Edit (map)
- **What:** Opens the Map Properties window for the chosen map.
- **Behavior:**
  - The fields are listed in RM-MPR-*.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-030 — New (map)
- **What:** Creates a new map entry.
- **Behavior:**
  - The new map is set up in the same Map Properties window.
  - The page does not say where the new map lands in the tree. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-031 — Load Sample Map
- **What:** Creates a new map from bundled sample data.
- **Behavior:**
  - A window lists the sample maps by name. You pick one, review its details, and confirm with OK, which adds it as new map data.
  - The help does not list the sample maps. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-032 — Copy (map)
- **What:** Puts the chosen map's data on the clipboard.
- **Behavior:**
  - Nothing more is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-033 — Paste (map)
- **What:** Adds the map data on the clipboard as a new map.
- **Behavior:**
  - The help does not say whether events come along. The Edit-menu description (RM-MAP-025) says paste covers map data and map events. **Not verified** for the context-menu command itself.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-034 — Delete (map)
- **What:** Removes the chosen map's data.
- **Behavior:**
  - The help does not say what happens to child maps. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-035 — Shift (map)
- **What:** Moves all tiles of a map by an offset.
- **Behavior:**
  - You give a direction and a number of tiles, and every tile on the map moves accordingly.
  - The help does not give the allowed directions, the range, edge handling, or whether events move too. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-036 — Generate Dungeon
- **What:** Automatically draws a maze-style dungeon onto the chosen map.
- **Behavior:**
  - Open it from the Map List context menu.
  - The dungeon fills the chosen map's current size. Small maps give poor results, so enlarge the map first for a larger dungeon.
  - Type has two choices. "Rooms" makes rooms joined by corridors. "Maze" makes corridors only.
  - Option checkboxes: "Add Margins" leaves a border on all four sides, and "Wide Passages" makes the corridors wider.
  - The Wall tile must come from the tileset's A4 (wall) tiles. The Floor tile must come from the tileset's A5 tiles.
  - Defaults, size limits and preview behavior are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-MAP-037 — Save as Image
- **What:** Exports the chosen map's rendered design to an image file.
- **Behavior:**
  - The help does not give the format, scale, or which layers are included. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

### RM-MAP-038 — Map grouping (map tree hierarchy)
- **What:** Maps can be nested under other maps in the Map List so they are easier to organize.
- **Behavior:**
  - Dragging one map onto another makes it a child of that map, appended at the end of its children. Example: building interiors under a town.
  - To move a child back to the top level, drag it onto the root folder that carries the project name.
  - Nesting only changes how the list is displayed. It has no effect on a map's design or settings.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html

---

## MPR: map properties

### RM-MPR-001 — Map Properties window
- **What:** The settings dialog for a single map.
- **Behavior:**
  - It opens when you create a new map, or when you right-click a map and choose Edit.
  - It covers size, tileset, BGM and encounter conditions, and other gameplay behavior.
  - Its sections are Basic Settings, Parallax Background, Memo and Encounter.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-002 — Name
- **What:** The map's internal name.
- **Behavior:**
  - It is used only in the editor, for example as the Map List label. It has no effect in-game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-003 — Display Name
- **What:** The map name the player sees.
- **Behavior:**
  - It is shown when the player arrives on the map.
  - Blank-value behavior is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-004 — Tileset
- **What:** Chooses which tileset (database entry) the map is drawn with.
- **Behavior:**
  - Changing it re-skins the map immediately (see RM-MAP-001).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07.html

### RM-MPR-005 — Width / Height
- **What:** The map's dimensions in tiles.
- **Behavior:**
  - Each can be set from 0 to 256.
  - Shrinking a map deletes everything outside the new bounds.
  - Which side grows or shrinks (the anchor) is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-006 — Scroll Type
- **What:** Sets whether, and on which axes, the map wraps around.
- **Behavior:**
  - "No Loop": edges do not wrap.
  - "Loop Vertically": the top edge joins the bottom edge.
  - "Loop Horizontally": the left edge joins the right edge.
  - "Loop Both": both pairs of edges join.
  - The default value is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-007 — Specify Battleback
- **What:** Overrides the automatic choice of battle background for battles that start on this map.
- **Behavior:**
  - When it is on, the battle background is built from two images chosen together. The Asset Standards page describes these as a floor image (battlebacks1) and a wall image (battlebacks2); see RM-AST-003.
  - When it is off, battles on field-type maps automatically use a background that matches the terrain. Other maps use a processed image of the map screen. The kind of processing is not described.
  - The Tileset Mode (Field or Area) affects how battle backgrounds are handled (RM-TIL-004).
  - The terrain-to-background mapping table is not given. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-MPR-008 — Encounter Steps
- **What:** How often random battles happen on this map.
- **Behavior:**
  - It is the average number of steps between random encounters, from 1 to 999. One step is one tile of movement.
  - Lower numbers mean more frequent battles.
  - The default value and the exact random formula are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-009 — Autoplay BGM
- **What:** Background music that starts automatically on this map.
- **Behavior:**
  - It is a checkbox plus a chosen music file. When enabled, the chosen BGM starts automatically while the player is on the map.
  - If the map BGM and the battle BGM are the same track, the music keeps playing through the switch to and from battle instead of restarting.
  - Volume, pitch and pan options are not described on this page. **Not verified** here.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-010 — Autoplay BGS
- **What:** A background sound (ambience) loop that starts automatically on this map.
- **Behavior:**
  - It is a checkbox plus a chosen sound file. When enabled, the chosen BGS starts automatically while the player is on the map.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-011 — Disable Dashing
- **What:** A per-map toggle that turns off dashing.
- **Behavior:**
  - When it is checked, the player cannot dash anywhere on this map.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-012 — Parallax Background (image)
- **What:** A background image shown wherever the map's tiles are transparent.
- **Behavior:**
  - Choose the image with the [...] button.
  - The images live in img/parallaxes (RM-AST-006).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MPR-013 — Parallax looping and auto-scroll
- **What:** Controls how the parallax background moves.
- **Behavior:**
  - With [Loop Horizontally] or [Loop Vertically] enabled, the parallax scrolls as the player moves.
  - Each loop direction has a scroll value from −32 to 32.
    - 0 means no automatic scrolling.
    - Positive values drift the image left or up. Negative values drift it right or down.
    - The larger the absolute value, the faster it moves.
  - The page uses the same bracketed loop names as Scroll Type. Whether these are separate parallax checkboxes or are tied to the map's Scroll Type was not clear from the fetched text. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-014 — Parallax: Show in the Editor
- **What:** Previews the parallax inside the Map View.
- **Behavior:**
  - When it is on, the parallax is drawn in the editor. The help warns the editor preview may not match what the game shows.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-015 — Parallax "!" filename prefix
- **What:** A filename convention that pins a parallax to the map.
- **Behavior:**
  - If the parallax image's filename starts with "!", its parallax factor becomes 0. It moves exactly with the map, so it acts like a painted floor or overlay instead of a distant background.
  - The Asset Standards page says the same thing: the file is treated as a floor with no parallax.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MPR-016 — Memo (note)
- **What:** Free-text developer notes for the map.
- **Behavior:**
  - Used only in the editor. The page does not say whether plugins can read it from this page. **Not verified** here.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-017 — Encounter list (management)
- **What:** The list of enemy troops that can appear in random battles on this map.
- **Behavior:**
  - Double-click an empty row to add an entry.
  - Clicking a registered entry gives a context menu with Copy and Delete.
  - Each entry has three fields: Troop, Weight and Scope (RM-MPR-018 to RM-MPR-020).
  - The help gives no limit on the number of entries. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-018 — Encounter: Troop
- **What:** Which troop (database entry) this encounter row spawns.
- **Behavior:**
  - Nothing more is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-019 — Encounter: Weight
- **What:** The relative chance of picking this troop.
- **Behavior:**
  - Range 0 to 100.
  - A troop's chance is its weight divided by the total weight of all eligible entries. Example: weights 9, 7 and 4 give 45%, 35% and 20%.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-020 — Encounter: Scope (Entire Map / Region IDs)
- **What:** Where on the map the encounter row applies.
- **Behavior:**
  - "Entire Map" means the troop can appear anywhere.
  - "Specify by Region ID" limits it to cells painted with the listed region IDs, up to 3 IDs per entry.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

### RM-MPR-021 — Regions (data model)
- **What:** Per-cell integer tags painted in region mode.
- **Behavior:**
  - IDs run from 1 to 255, and each cell holds at most one ID.
  - The main documented use is limiting encounters (RM-MPR-020).
  - Painting is described in RM-MAP-023.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

---

## TIL: tileset settings and tile kinds

### RM-TIL-001 — Tileset data (role)
- **What:** A database entry that bundles tile images with per-tile gameplay flags.
- **Behavior:**
  - Each tile gets an image plus behavior, such as passability.
  - You can use your own artwork by making a tileset that points at your own images.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-002 — Vehicle passability rules
- **What:** How boats, ships and the airship (plane) interact with tiles.
- **Behavior:**
  - Whether boats and large ships can enter a tile depends on where the tile sits in the tileset (its sheet or block), not on a per-tile flag. See RM-TIL-018 to RM-TIL-023.
  - The plane can fly over every tile.
  - The plane can land only on tiles the player can walk on. Even then, it cannot land on bush-flagged tiles or on any A1 tile.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-003 — Tileset Name
- **What:** Internal name of the tileset.
- **Behavior:**
  - Used only in the editor.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-004 — Mode (Field Type / Area Type)
- **What:** Declares what the tileset is for. This changes some lower-layer rules and how battle backgrounds are handled.
- **Behavior:**
  - Field Type is for world maps (sea, land and so on). Area Type is for everything else.
  - The mode changes the A2 rules (RM-MAP-021, RM-TIL-025, RM-TIL-026) and how automatic battle backgrounds are handled (RM-MPR-007).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-005 — Images (sheet slots)
- **What:** The image files assigned to the tileset.
- **Behavior:**
  - There are nine slots: A1, A2, A3, A4, A5, B, C, D and E.
  - Each slot's button opens an image picker ("Select an Image"). The chosen sheet appears in the Tile List.
  - Sheet sizes are covered in RM-AST-010.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-006 — Tile List (flag editor interaction)
- **What:** A grid for editing per-tile flags.
- **Behavior:**
  - Tabs A to E switch the sheet. Tab A shows A1 through A5 one after another.
  - Each tile shows a mark for the flag being edited, and clicking the mark cycles its value.
  - For flags with three or more values (Passage, Terrain Tag), right-click cycles in the reverse direction.
  - Click-dragging applies the change to every tile the cursor passes over.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-007 — Passage (○ / × / ☆)
- **What:** Basic walkability of a tile.
- **Behavior:**
  - ○ means the tile can be entered.
  - × means it cannot be entered.
  - ☆ means it can be entered and is drawn above characters, so characters disappear behind building tops and similar. ☆ cannot be used on tab A tiles.
  - Stacking resolution is covered in RM-MAP-020.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-008 — Passage (4 Dir)
- **What:** Per-direction entry permissions for a tile.
- **Behavior:**
  - Arrow marks show the directions from which the tile can be entered. A missing arrow blocks entry from that side.
  - Typical use is one-way edges, such as a cliff lip that suggests height.
  - It is kept in sync with the basic Passage flag: changing Passage updates the directional arrows.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-009 — Ladder
- **What:** Climbable tile flag.
- **Behavior:**
  - While a character is on the tile, it always faces up, so it looks as if it is climbing a ladder or rope.
  - The editor mark is a ladder icon.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-010 — Bush
- **What:** Partial-concealment flag for tall grass and similar.
- **Behavior:**
  - The lowest 12 pixels of a character standing on the tile are drawn semi-transparent.
  - On some A1 to A4 tiles the effect may not appear, depending on the image (see the forest-type rule in RM-TIL-017).
  - The plane cannot land on bush tiles (RM-TIL-002).
  - The editor mark is two wavy lines.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-011 — Counter
- **What:** Counter-top flag that lets the player talk across the tile.
- **Behavior:**
  - Events can be triggered from the far side of a counter tile, for example a shopkeeper behind a desk.
  - On A2 tiles, the flag also makes the tile act as a table autotile, and the drawn pattern is shifted 12 pixels down.
  - The editor mark is a diamond.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-012 — Damage Floor
- **What:** Hazard flag.
- **Behavior:**
  - Characters that walk across the tile take damage. Examples are poison swamps and thorny plants.
  - The editor mark is two triangles.
  - The amount of damage is not stated on this page. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-013 — Terrain Tag
- **What:** A small general-purpose number on each tile.
- **Behavior:**
  - Values run from 0 to 7. The engine assigns them no built-in meaning.
  - The game reads them with the "Get Location Info" event command.
  - When layers overlap, a nonzero tag on an upper layer takes precedence.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-014 — Note (tileset)
- **What:** Free-text notes field.
- **Behavior:**
  - For development notes.
  - Its right-click menu has "Plugin Help..." to show plugin documentation.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html

### RM-TIL-015 — Tile basics (size and classes)
- **What:** The tile grid unit and how tile classes are grouped.
- **Behavior:**
  - Tiles are 48×48 pixels.
  - There are five classes, A to E. A is the lower layer. B to E are upper layers.
  - A is split into five sub-sheets, A1 to A5. Most of A is autotiles, whose borders are generated automatically.
  - Some tile behavior depends on the tileset Mode (RM-TIL-004).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-016 — Autotile source structure
- **What:** How one autotile is laid out in its source image (described functionally).
- **Behavior:**
  - Each autotile takes a 6-tile area of the sheet, 2 tiles across by 3 down. The 2×3 shape is inferred from the 6-tile count and the sheet sizes; the help shows the layout only in a figure.
  - The block contains three kinds of piece:
    - A representative image, shown in the palette.
    - A piece with borders at all four corners.
    - A 3×3-style "group" piece: a center plus one neighbor in each of the 8 directions, giving the edge and corner borders.
  - The editor assembles each placed cell from these pieces according to which neighbors share the same autotile.
  - Some sheets (A3, and the lower part of A4) use only the group pattern. See RM-TIL-027 and RM-TIL-028.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-017 — Forest-type autotile
- **What:** A variant autotile whose bush effect is suppressed at the edges.
- **Behavior:**
  - An autotile counts as "forest type" if a specific cell of its source block is fully transparent. The help puts it at (8,8) from the bottom-right; the exact pixel or cell convention appears only in a figure.
  - If a forest-type tile also has the Bush flag, characters are not drawn semi-transparent on its 8 boundary pieces, which include the bottom-left and bottom-right edges.
  - The full list of the 8 boundary pieces is shown only in a figure. **Not verified** in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-018 — A1 sheet (animated water)
- **What:** The lower-layer sheet for animated water tiles.
- **Behavior:**
  - It is made of five-pattern blocks, called Blocks A to E.
  - A1 tiles do not draw border lines against each other, even when they touch.
  - Boats and ships can travel only on A1 tiles.
  - If an A1 tile's passage is set so the player can walk on it, boats and ships can no longer enter it.
  - The plane can never land on A1 (RM-TIL-002).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-019 — A1 Block A: Ocean
- **What:** Animated shallow-sea autotile.
- **Behavior:**
  - Animated by setting three basic autotile frames side by side.
  - Both boats and ships can travel on it; no restriction is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-020 — A1 Block B: Deep Sea
- **What:** Animated deep-water autotile.
- **Behavior:**
  - Of the A1 blocks, only this one forms border lines where it meets ocean tiles.
  - Its transparent parts are filled with the Block A ocean.
  - Animated with three side-by-side frames, like Block A.
  - The page says boats cannot travel on it. For Blocks C and E it says "boats and ships", so the inference is that ships can travel on Block B. That inference is **not verified** in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-021 — A1 Block C: Ocean Decoration
- **What:** Autotiles that decorate the ocean, such as rocks or reefs placed on sea.
- **Behavior:**
  - Its transparent parts are filled with the Block A ocean.
  - Neither boats nor ships can travel on it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-022 — A1 Block D: Water
- **What:** Animated autotile for inland water (ponds, lakes and so on).
- **Behavior:**
  - Animated with three side-by-side basic frames.
  - No vehicle restriction is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-023 — A1 Block E: Waterfall
- **What:** Animated waterfall autotile.
- **Behavior:**
  - Two tiles side by side make up its group pattern, and three of those stacked vertically make the animation frames.
  - Neither boats nor ships can travel on it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-024 — A2 sheet (ground)
- **What:** The lower-layer sheet for ground autotiles: grass, dirt, floors and so on.
- **Behavior:**
  - It is made of four rows of two-pattern blocks stacked vertically.
  - It is the only sheet whose rules change with the tileset Mode.
  - A2 tiles that have the Counter flag act as table autotiles, drawn shifted 12 pixels down (RM-TIL-011).
  - Base and Decoration split: see RM-MAP-021.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-TIL-025 — A2 under Field Type
- **What:** How A2 blocks behave in a world-map tileset.
- **Behavior:**
  - Block A holds four-pattern autotiles. In use they appear as pattern 1 alone, patterns 1+2 overlaid, pattern 3 alone, or patterns 3+4 overlaid.
  - Block B holds four special overlay patterns that can be placed over Block A tiles. This matches the base-swap rule in RM-MAP-021.
  - Mapping Block A to the left half (Base columns 1–4) and Block B to the right half (Decoration columns 5–8) is inferred by matching this page with RM-MAP-021. **Not verified** in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-026 — A2 under Area Type
- **What:** How A2 blocks behave in an area (non-world-map) tileset.
- **Behavior:**
  - Block A holds four patterns that can be placed over Block B tiles.
  - Block B holds four patterns that can be placed over Block A tiles.
  - The fetched text did not settle which half is "A" and which is "B" under Area Type. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-027 — A3 sheet (buildings)
- **What:** Lower-layer autotiles for building exteriors.
- **Behavior:**
  - It is a grid of autotiles, 8 across by 4 down, and each uses only the group pattern.
  - Stacking two or more vertically produces an autoshadow (RM-TIL-031).
  - Separate roof and wall roles are not spelled out in the text. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-TIL-028 — A4 sheet (walls)
- **What:** Lower-layer autotiles for walls, including dungeon walls.
- **Behavior:**
  - It holds autotiles 8 across by 3 down that use the full basic autotile structure. Directly beneath each of these sits a tile that uses only the group pattern. The exact interleaving is shown only in a figure. **Not verified.**
  - Autoshadows follow the same rule as A3 (RM-TIL-031).
  - Generate Dungeon takes its wall tile from A4 (RM-MAP-036).
  - Wall-top and wall-face roles are not named in the text. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-TIL-029 — A5 sheet (normal lower tiles)
- **What:** Ordinary, non-auto lower-layer tiles.
- **Behavior:**
  - The grid is 8 columns by 16 rows, and every tile is a plain single tile.
  - Rows 3, 5 and 7 (counted from the top) can also serve as floors for dungeon generation.
  - Generate Dungeon takes its floor tile from A5 (RM-MAP-036).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-TIL-030 — B–E sheets (upper layers)
- **What:** Sheets for upper-layer objects.
- **Behavior:**
  - Each is a 16×16 grid of plain tiles.
  - The top-left tile of sheet B must be left blank, because it acts as the "empty upper tile" or eraser (RM-MAP-015).
  - The ☆ passage setting is allowed on these sheets (RM-TIL-007).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-TIL-031 — Autoshadow generation rule (A3/A4)
- **What:** When the automatic shadow is drawn.
- **Behavior:**
  - When two or more A3 or A4 tiles are stacked vertically, a shadow is drawn on the tile immediately to their right.
  - No shadow is drawn if that right-hand neighbor is anything other than an A2 ground tile or an A5 tile.
  - The help adds "(excluding Block C)" to the A2 case, but A2 is described with only Blocks A and B, so this exception is unexplained. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

---

## AST: asset standards

### RM-AST-001 — Asset placement rules (general)
- **What:** How user-supplied assets are added to a project.
- **Behavior:**
  - Any asset type can be replaced with your own files. Copy the file into the matching asset folder, and the editor then lists it for selection.
  - Subfolders inside asset folders are supported and can be browsed.
  - Folder names should use half-width alphanumeric characters.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-002 — Image file format
- **What:** The format for all image assets.
- **Behavior:**
  - PNG is the image format used for every image category on the page.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-003 — Battlebacks (img/battlebacks1, img/battlebacks2)
- **What:** Two-part battle background images.
- **Behavior:**
  - Each image is 1000×740 px.
  - battlebacks1 mainly holds floors (the ground part). battlebacks2 mainly holds walls (the backdrop part). Any floor can be combined with any wall.
  - Front-view battles crop off part of the top. Side-view battles crop off part of the bottom.
  - Used by Specify Battleback (RM-MPR-007).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-004 — Front-view enemies (img/enemies)
- **What:** Enemy battler images for front-view battles.
- **Behavior:**
  - Any size is allowed.
  - Use even pixel widths, because odd widths can render blurry.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-005 — Faces (img/faces)
- **What:** Portrait images for menus and message windows.
- **Behavior:**
  - Each face is 144×144 px.
  - One file holds 8 faces in a grid of 4 columns by 2 rows, so a sheet is 576×288 px by arithmetic. That total size is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-006 — Parallaxes (img/parallaxes)
- **What:** Map background images (RM-MPR-012).
- **Behavior:**
  - Any size is allowed.
  - For a seamless loop, opposite edges (top and bottom, left and right) must tile into each other.
  - A filename starting with "!" makes the image act as a fixed floor with no parallax (RM-MPR-015).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-007 — Pictures (img/pictures)
- **What:** Free images shown through picture event commands.
- **Behavior:**
  - Any size is allowed.
  - Use even widths to avoid blurring.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-008 — Side-view actors (img/sv_actors)
- **What:** Actor battler sheets for side-view battles. There is one character per file.
- **Behavior:**
  - Each sheet is a grid of 9 columns by 6 rows, giving 54 frames. The standard sheet is 576×384 px, so each frame is 64×64 px by arithmetic.
  - Every run of three adjacent frames in a row is one motion, so each row holds three motions.
  - Looping idle-type motions play the three frames forward and back: 1, 2, 3, 2, and repeat.
  - One-shot motions such as taking damage play 1, 2, 3 once.
  - The help gives the motion names and their slot positions only in a figure. **Not verified** in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-009 — Side-view enemies (img/sv_enemies)
- **What:** Enemy battler images for side-view battles.
- **Behavior:**
  - Any size is allowed. Even widths are recommended.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-010 — Tileset images (img/tilesets)
- **What:** Sheet sizes for each tileset slot. All tiles are 48×48 px.
- **Behavior:**
  - A1 is 768×576 px (animated water; RM-TIL-018 to RM-TIL-023).
  - A2 is 768×576 px (ground; RM-TIL-024 to RM-TIL-026).
  - A3 is 768×384 px (buildings; RM-TIL-027).
  - A4 is 768×720 px (walls; RM-TIL-028).
  - A5 is 384×768 px: 8 by 16 plain tiles (RM-TIL-029).
  - B, C, D and E are 768×768 px each: 16 by 16 plain tiles. The top-left cell of B stays blank (RM-TIL-030).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-011 — Title screen images (img/titles1, img/titles2)
- **What:** Two-layer title screen artwork.
- **Behavior:**
  - Each image is 816×624 px.
  - titles1 holds the main background picture. titles2 holds an overlay such as a frame. The two are combined to make the title screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-012 — Character sheets (img/characters)
- **What:** Walking sprites for the player, followers and events.
- **Behavior:**
  - The standard frame is 48×48 px, but other sizes are allowed. The editor derives the frame size from the file itself: frame width is 1/12 of the image width and frame height is 1/8 of the image height.
  - Each character has 12 frames: 4 facing directions (up, down, left, right) × 3 walk-cycle patterns, set out in a fixed order. That order appears only in a figure.
  - A standard file holds 8 such character blocks, 4 across by 2 down. This matches the 1/12 and 1/8 rule (4 characters × 3 patterns across, 2 characters × 4 directions down). The help's text wording here is garbled and the layout appears in a figure, so the 4×2 reading is inferred from the stated 1/12 and 1/8 rule.
  - Characters are drawn 6 px higher than their tile, so they sit naturally in front of buildings.
  - The direction row order and pattern order inside a block are **not verified** in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-013 — Character filename prefix "$"
- **What:** Marks a file as holding one character.
- **Behavior:**
  - A leading "$" means the file contains a single character's 12 frames instead of a block of 8 characters.
  - The derived frame-size rule for "$" files (presumably 1/3 of the width and 1/4 of the height) is not stated. **Not verified.**
  - It can be combined with "!".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-014 — Character filename prefix "!"
- **What:** Marks a file as an object sprite: doors, chests and the like.
- **Behavior:**
  - A leading "!" turns off the 6-px upward offset, so the sprite sits flush on its tile.
  - It also turns off the semi-transparent effect on bush tiles.
  - It can be combined with "$". The help does not state the combined order on this page. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-015 — System image: Balloon.png
- **What:** Speech or emotion balloon icons shown over characters.
- **Behavior:**
  - The page mentions a user-definable area in the image, but the fetched text did not make clear which area. Cell size and layout are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-016 — System image: ButtonSet.png
- **What:** On-screen touch control buttons.
- **Behavior:**
  - Covers actions such as opening the menu, cancel or back, and changing quantities in shops.
  - Layout is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-017 — System image: GameOver.png
- **What:** The image shown on the Game Over screen.
- **Behavior:**
  - Size is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-018 — System image: IconSet.png (icons)
- **What:** One sheet holding all icons for items, weapons, skills and similar.
- **Behavior:**
  - Icon cell size and grid width are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-019 — System image: Shadow1.png
- **What:** The ground shadow drawn under the plane (airship) while flying.
- **Behavior:**
  - Nothing more is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-020 — System image: Shadow2.png
- **What:** The shadow drawn under actors in side-view battle.
- **Behavior:**
  - Nothing more is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-021 — System image: States.png
- **What:** State or status overlay graphics for actors in side-view battle.
- **Behavior:**
  - Layout is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-022 — System images: Weapons1.png, Weapons2.png
- **What:** Weapon swing graphics shown during actors' normal attacks in side-view battle.
- **Behavior:**
  - Each file holds 12 weapon types.
  - Per-type frame layout is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-023 — System image: Weapons3.png
- **What:** A third weapon graphics sheet for side-view normal attacks.
- **Behavior:**
  - Its contents are left for the user to define.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-024 — System image: Window.png
- **What:** The window skin: a set of image parts used to build every UI window.
- **Behavior:**
  - The breakdown of its parts is not given on this page. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-025 — Audio (music and sound effects)
- **What:** The audio format for music and sound effects.
- **Behavior:**
  - Audio files are Ogg Vorbis (.ogg).
  - If a file carries LOOPSTART and LOOPLENGTH metadata tags, playback loops using those values.
  - The tag units (samples or seconds), the behavior when no tags are present, the audio folder names and the per-category split (BGM, BGS, ME, SE) are not stated on this page. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-026 — Movies (video files)
- **What:** Video files played by the game.
- **Behavior:**
  - Two formats are used: WebM (.webm) and MP4 (.mp4, H.264). Both copies must have the same base filename.
  - Windows and Mac builds need only the WebM file.
  - Web, Android and iOS builds need both files, because supported formats differ by platform.
  - The movie folder name is not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-AST-027 — Animations (particle effects)
- **What:** Animation entries in the database point at an Effekseer particle-effect file. The Asset Standards page does not list this asset kind.
- **Behavior:**
  - The Animation database entry picks an Effekseer effect file.
  - The animation can have sound cues and flash cues placed on specific frames. Flash duration is set in 1/60-second units.
  - The effect folder, file extension and any legacy sprite-sheet animation format are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html

### RM-AST-028 — Fonts
- **What:** Game font files. The Asset Standards page does not list them.
- **Behavior:**
  - The System 2 database settings include Main Font Filename, Number Font Filename (for digits), Fallback Font (for glyphs missing from the main font) and Font Size.
  - The font folder, supported formats and defaults are not stated. **Not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-AST-029 — Resource Manager (asset import)
- **What:** An editor tool for managing the project's asset files.
- **Behavior:**
  - It organizes project resources (images, audio and so on) by folder and can preview images.
  - Imported files must follow the Asset Standards.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

---

## Not verified

These points could not be confirmed on any fetched official page. Each was either not stated or shown only in a figure.

**Map editor**
1. The default layer assignment for Layers 1–4, meaning which tile class goes on which layer (RM-MAP-022).
2. The exact display-order wording for two ☆ tiles stacked together (RM-MAP-020).
3. The zoom steps in the Scale menu (RM-MAP-024). Whether Redo exists (RM-MAP-025).
4. Where a new map is placed in the Map List (RM-MAP-030).
5. The list of sample maps (RM-MAP-031).
6. Whether context-menu Paste includes events (RM-MAP-033).
7. Whether deleting a map also deletes its child maps (RM-MAP-034).
8. For Shift: the direction options, tile-count range, edge or wrap handling, and whether events move too (RM-MAP-035).
9. For Generate Dungeon: defaults and size limits (RM-MAP-036).
10. For Save as Image: format, scale and layers included (RM-MAP-037).

**Map properties**
11. Display Name behavior when left blank (RM-MPR-003).
12. Which side is anchored when a map is resized (RM-MPR-005).
13. The default Scroll Type (RM-MPR-006).
14. The automatic battleback terrain mapping, and what "processed map screen image" means (RM-MPR-007).
15. The default for Encounter Steps and the exact encounter randomisation formula (RM-MPR-008).
16. Volume, pitch and pan options for Autoplay BGM/BGS on the Map Properties page (RM-MPR-009, RM-MPR-010).
17. Whether the parallax loop checkboxes are separate from the map Scroll Type (RM-MPR-013).
18. Whether plugins can read the map Memo (RM-MPR-016).
19. The maximum number of encounter entries (RM-MPR-017).

**Tilesets**
20. Damage Floor damage amount (RM-TIL-012).
21. The forest-type 8-tile boundary list, and the exact convention for the "(8,8)" position (RM-TIL-017).
22. Which half of A2 is Block A versus Block B under Area Type (RM-TIL-026).
23. A3 roof and wall roles (RM-TIL-027). A4 wall-top and wall-face roles (RM-TIL-028).
24. The meaning of "excluding Block C" in the A2 autoshadow exception (RM-TIL-031).
24a. Whether, under Field Type, A2 Block A is the left half and Block B the right half (RM-TIL-025).
24b. Whether ships can travel on A1 Block B (RM-TIL-020).
24c. The exact interleaving of A4 basic-structure and group-only rows (RM-TIL-028).
24d. The exact cell layout of an autotile's 6-tile source block. The 2×3 shape is inferred (RM-TIL-016).

**Assets**
25. The character sheet's direction-row and pattern order in text (RM-AST-012).
26. The frame-size rule for "$" files (RM-AST-013).
27. The required order when combining "$" and "!" (RM-AST-014).
28. Side-view actor motion names and slot positions (RM-AST-008).
29. Layouts and sizes for Balloon.png, ButtonSet.png, GameOver.png, IconSet.png (icon cell size), States.png, the Weapons1/2 frame layout, and the Window.png part breakdown (RM-AST-015 to RM-AST-024).
30. Audio folder names, the BGM/BGS/ME/SE split, loop-tag units and behavior without loop tags (RM-AST-025).
31. The movie folder name (RM-AST-026).
32. The animation (Effekseer) folder and extension, and any legacy sprite-sheet animation images (RM-AST-027).
33. The font folder, formats and defaults (RM-AST-028).
34. The help table of contents: 01.html returned no TOC text through WebFetch, so related pages were found by direct URL and from in-page links.
