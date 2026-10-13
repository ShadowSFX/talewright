# RPG Maker MZ: Project, Editor, Aid Tools, Test Play, Plugins, Deployment (Talewright clean-room inventory, slice 01)

Researched 2026-10-13. Everything below is written in our own words from official pages fetched in this session. UI field, command, menu, folder and annotation names are given exactly because they are identifiers. Nothing here is copied help prose, and there is no code.

Method note: pages were read with WebFetch, which passes page text through a summariser. Text that exists only inside screenshots could not be read, so anything shown only in images is listed under "Not verified".

## 1. Pages covered

Base: `https://rpgmakerofficial.com/product/MZ_help-en/`

### Required slice

| # | Page | URL | Loaded |
|---|---|---|---|
| 1 | About RPG Maker MZ | https://rpgmakerofficial.com/product/MZ_help-en/01_01.html | Yes |
| 2 | Main Features of RPG Maker MZ | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html | Yes |
| 3 | MV Data Conversion | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html | Yes |
| 4 | Time Progression Battles | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html | Yes |
| 5 | Animations | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_04.html | Yes |
| 6 | Improvements to Artistic Expressions | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html | Yes |
| 7 | Game Control Improvements | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html | Yes |
| 8 | System Requirements | https://rpgmakerofficial.com/product/MZ_help-en/01_01_02.html | Yes |
| 9 | Getting Started Making Your Game | https://rpgmakerofficial.com/product/MZ_help-en/01_02.html | Yes |
| 10 | Basic Editor Controls | https://rpgmakerofficial.com/product/MZ_help-en/01_03.html | Yes |
| 11 | Menu Bar Content | https://rpgmakerofficial.com/product/MZ_help-en/01_04.html | Yes |
| 12 | How to Use Aid Tools | https://rpgmakerofficial.com/product/MZ_help-en/01_05.html | Yes |
| 13 | Playing Your Game | https://rpgmakerofficial.com/product/MZ_help-en/01_06.html | Yes |
| 14 | Side-View Character Standards | https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html | Yes (short page) |
| 15 | Output Formats | https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html | Yes |
| 16 | Enabling Plugins | https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html | Yes (short page) |
| 17 | Using Official Plugins | https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html | Yes |
| 18 | RPG Maker MZ Core API (landing) | https://developer.rpgmakerweb.com/rpg-maker-mz/ | Yes (class index only) |

### Supplementary official pages (linked sub-pages, or needed for the plugin, Battle Test and localization questions)

| Page | URL | Loaded |
|---|---|---|
| Help index | https://rpgmakerofficial.com/product/MZ_help-en/01.html | Yes, but the table of contents is loaded dynamically and was not captured |
| Help root | https://rpgmakerofficial.com/product/MZ_help-en/index.html | Yes, but navigation is dynamic and nothing was captured |
| "New Functions" index probe | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01.html | **No (HTTP 404)**. This was an extra probe, not part of the required list |
| Editing Map Designs (linked from 01_04 Draw/Layer) | https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html | Yes |
| Troop Settings (Battle Test) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html | Yes |
| System 1 (linked from 01_01_01_03 and 01_01_01_06) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html | Yes |
| System 2 (linked from 01_01_01_05) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html | Yes |
| Type Settings (localization check) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html | Yes |
| Terms Settings (localization check) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html | Yes |
| Event Commands: Advanced (Script / Plugin Command) | https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html | Yes |
| Asset Standards (linked from 01_05 Resource Manager) | https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html | Yes |
| API class pages | https://developer.rpgmakerweb.com/rpg-maker-mz/Graphics.html, Utils.html, Tilemap.html, WebAudio.html, Window.html, Input.html, Stage.html, Bitmap.html, Sprite.html | Yes (all 9) |
| Official product page | https://www.rpgmakerweb.com/products/rpg-maker-mz | Yes |
| Official blog: MZ Preview #3 | https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command | Yes |
| Official blog: Beginner's Guide to Plugins (Part 1) | https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1 | Yes |
| Official plugin annotation page | https://tkool.jp/mz/plugin/make/annotation.html, which returns 301 to https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html | Yes (via the redirect target) |
| Official Japanese plugin course landing | https://rpgmakerofficial.com/product/mz/plugin/ | Yes (landing text only, no links captured) |
| Plugin "make" folder probe | https://rpgmakerofficial.com/product/mz/plugin/make/ | **No (HTTP 404)**. This was an extra probe |

Note on area use: the help's "Playing Your Game" page (runtime controls, title/menu/battle/shop menus) is filed under **TST**. The high-level Core API architecture is filed under **PLG** because that documentation is the plugin author's reference.

---

## 2. Items

### PRJ: Project structure, new project, files

### RM-PRJ-001 — Product scope
- **What:** RPG Maker MZ is an authoring tool for making role-playing games. Finished games can be played by people who do not own the tool.
- **Behaviour:**
  - Players need no editor licence to play a finished game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01.html

### RM-PRJ-002 — New Project
- **What:** Creates a new project, which is pre-filled as a sample game, from a short dialog.
- **Behaviour:**
  - You can start it from the toolbar [New Project] button or from File > New Project.
  - Starting it closes any project that is currently open (per the File menu description).
  - Dialog fields:
    - Project name: half-width alphanumeric characters are recommended.
    - [Title]: the game's display name.
    - [Location]: shows the save folder. The [Choose...] button beside it picks a different folder.
  - Pressing [OK] generates the project as a sample game from these settings, ready to edit.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-PRJ-003 — Save Project
- **What:** Writes the current edits over the saved project.
- **Behaviour:**
  - You can run it from the toolbar [Save Project] button or from File > Save Project.
  - The save overwrites the project in place, with no versioning described.
  - The help recommends saving and closing the program before taking a break.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-PRJ-004 — Open Project
- **What:** Opens an existing project by choosing its project file.
- **Behaviour:**
  - You can run it from the toolbar [Open Project] button or from File > Open Project.
  - In the file dialog, you pick the project file, which is shown as [Game] (`game.rmmzproject`), and press [Open].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-PRJ-005 — Close Project
- **What:** Closes the active project.
- **Behaviour:**
  - If there are unsaved changes, the editor asks whether to save them first.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-PRJ-006 — Project is a self-contained folder (backup and deletion)
- **What:** The whole project lives in the folder chosen at creation.
- **Behaviour:**
  - To back up a project, you copy the entire folder to other media (a USB stick, for example).
  - To delete a project, you delete its folder like any other file. There is no in-editor delete command.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_02.html

### RM-PRJ-007 — Three core data kinds: Map, Events, Database
- **What:** The help frames game content as three kinds of data.
- **Behaviour:**
  - [Map] holds the game's settings and locations.
  - [Events] holds what happens in the game.
  - [Database] holds definitions such as characters, items and magic.
  - You can work on them in any order. For first-time users, the help suggests starting with a map, then adding events and characters.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_02.html

### RM-PRJ-008 — Project data and asset file layout (as revealed by the MV conversion table)
- **What:** The MV-to-MZ migration table lists the project-relative folders and data files that MZ uses.
- **Behaviour:**
  - Asset folders:
    - `audio`
    - `img/battlebacks1`, `img/battlebacks2`, `img/characters`, `img/enemies`, `img/faces`, `img/parallaxes`, `img/pictures`, `img/sv_actors`, `img/sv_enemies`, `img/tilesets`, `img/titles1`, `img/titles2`, `img/system`
    - `img/animations` (MV-style animations only)
    - `icon`
    - `fonts`
  - Data files in `data/`:
    - `Actors.json`, `Classes.json`, `Skills.json`, `Items.json`, `Weapons.json`, `Armors.json`, `Enemies.json`, `Troops.json`, `States.json`, `Tilesets.json`, `CommonEvents.json`, `Animations.json`
    - `System.json`, which holds system settings, vocabulary, types, and switch and variable names.
    - `MapInfos.json`, plus one `MapXXX.json` per map, where XXX is the map ID.
  - Other folders:
    - `effects` is an asset folder that deployment can prune (see RM-DEP-007).
    - `js/plugins` holds plugin script files (see RM-PLG-004).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html

### RM-PRJ-009 — MV project conversion procedure
- **What:** An RPG Maker MV project can be brought into MZ by copying its files into a fresh MZ project. MZ converts them automatically the next time it starts.
- **Behaviour:**
  - Steps:
    1. Make file extensions visible in the OS.
    2. Back up the MV project.
    3. Create a new MZ project to receive the data.
    4. Close MZ. Conversion does not happen while the project is open.
    5. Copy the MV files and folders into the matching MZ paths, overwriting where needed.
    6. Start MZ. Conversion then runs automatically.
  - The help describes no confirmation dialog.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-PRJ-010 — MV conversion exclusions and warnings
- **What:** Some MV content should not be copied, or should be copied only partly.
- **Behaviour:**
  - System images: `ButtonSet.png`, `Damage.png`, `Loading.png` and `MadeWithMv.png` are not needed in MZ.
  - Animations: converting `Animations.json` together with `img/animations` is discouraged, because MV-style animations cannot be edited in MZ.
  - Fonts: MZ assigns fonts in the database (System 2 > Advanced Settings). Copy only the font files and leave out `gamefont.css`.
  - Plugins: MV plugins are not guaranteed to work in MZ. If MV plugins are present, the Plugin Manager shows a notice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

---

### EDT: Editor UI, controls, menu bar, modes, options

### RM-EDT-001 — Main Window
- **What:** The main editor screen appears once a project is open. It is used mainly for editing maps and events.
- **Behaviour:**
  - Other tools, such as the Database, open from this window's menus.
  - The window has seven numbered regions: Menu Bar, Toolbar, Tile Palette, Map List, Map View, Status Bar and Layer button (RM-EDT-002 to RM-EDT-008).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-002 — Menu Bar
- **What:** The menu row that gives access to all editor functions.
- **Behaviour:**
  - Menus documented on the page: [File], [Edit], [Mode], a second section also headed [Mode] that covers drawing tools (the toolbar/Draw menu, see RM-EDT-027), [Layer], [Scale], [Tool], [Game] and [Help].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-003 — Toolbar
- **What:** A row of one-click buttons for common functions.
- **Behaviour:**
  - Buttons named elsewhere in the help: [New Project], [Open Project], [Save Project], [Map] (mode), and the drawing tools.
  - The official blog notes a puzzle-piece icon that opens the Plugin Manager.
  - No complete button list is given.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1

### RM-EDT-004 — Tile Palette
- **What:** Shows the tiles that can be painted onto the current map.
- **Behaviour:**
  - Tabs along the bottom switch the tile set shown: A, B, C, D, E, and R.
  - The R tab switches to region editing (RM-EDT-026).
  - Clicking a tile selects it and draws a white frame around it.
  - Dragging across the palette selects a block of several tiles.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-005 — Map List
- **What:** A tree, in the lower-left of the window, of every map in the game.
- **Behaviour:**
  - Clicking a map shows it in the Map View.
  - Right-clicking a map's name opens a context menu that includes Generate Dungeon (RM-AID-012).
  - The product page says maps can be reordered in the tree, including among siblings at the same level.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-EDT-006 — Map View
- **What:** The main canvas showing the selected map.
- **Behaviour:**
  - Used for painting tiles and for choosing where events go.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-007 — Status Bar
- **What:** An information strip at the bottom of the window.
- **Behaviour:**
  - Shows details of the selected function, the map name and the cursor's map coordinates.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-008 — Layer button / Map Layer Selection
- **What:** In map editing mode, this picks which tile layer is being edited, so the other layers are left untouched.
- **Behaviour:**
  - Choices: Auto (the editor picks the layer as you draw), Layer 1, Layer 2, Layer 3, Layer 4.
  - Tiles on the chosen layer are highlighted, and only they can be changed. For example, you can re-floor a room on Layer 1 without disturbing decorations on higher layers.
  - The product page describes Automatic as keeping MV-style behaviour.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-EDT-009 — Text input fields
- **What:** Standard text entry.
- **Behaviour:**
  - Clicking the field shows a caret. Typing inserts text at the caret, and the arrow keys move it.
  - How characters are entered depends on the user's OS input method (IME).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-010 — Number input fields
- **What:** Numeric entry with spin buttons.
- **Behaviour:**
  - You can type half-width digits, or use the [∧] and [∨] buttons to step the value up and down.
  - MZ converts full-width digits to half-width automatically (see RM-NEW-024).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-EDT-011 — Radio buttons
- **What:** A choice of exactly one option from a group.
- **Behaviour:**
  - Clicking a circle fills it and makes that option the active one.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-012 — Check boxes
- **What:** An on/off toggle.
- **Behaviour:**
  - Clicking the square adds a tick, and clicking again removes it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-013 — Drop-down lists
- **What:** A choice of one item from a pop-up list.
- **Behaviour:**
  - Clicking the [∨] at the right edge opens the list.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-014 — List boxes
- **What:** Multi-entry lists inside dialogs.
- **Behaviour:**
  - Double-clicking an entry opens a sub-window to add or change it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-015 — [...] button
- **What:** A button beside a field that opens a separate configuration window for that item.
- **Behaviour:**
  - None documented beyond opening the window.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-016 — Dialog buttons [OK], [Cancel], [Apply]
- **What:** The standard way to commit or discard changes in dialogs.
- **Behaviour:**
  - [OK] commits the changes and closes the window.
  - [Cancel] throws away the changes and closes the window.
  - [Apply] commits the changes but leaves the window open.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-017 — Context-menu Search (maps and database)
- **What:** A text search opened by right-clicking in a map or a database list.
- **Behaviour:**
  - Choosing [Search] from the context menu opens a Search window.
  - After the first search, [Search Next] and [Search Previous] repeat it forwards and backwards.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-018 — File > Close RPG Maker MZ
- **What:** Quits the application.
- **Behaviour:**
  - If there are unsaved changes, the editor asks whether to save them first.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-019 — Edit > Undo
- **What:** Reverses the most recent edit.
- **Behaviour:**
  - Undo can step back through up to 20 operations.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-020 — Edit > Cut
- **What:** Removes the selected map data and map events and puts them on the clipboard.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-021 — Edit > Copy
- **What:** Puts the selected map data and map events on the clipboard without removing them.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-022 — Edit > Paste
- **What:** Inserts the clipboard contents as new map data or new map events.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-023 — Edit > Delete
- **What:** Deletes the selected map data and map events.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-024 — Mode > Map
- **What:** Switches the editor to Map Design editing, which is tile painting.
- **Behaviour:**
  - You can also switch with the toolbar [Map] button.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-025 — Mode > Event
- **What:** Switches the editor to Map Events editing.
- **Behaviour:**
  - The Map View shows grid lines one tile apart in this mode.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-026 — Region Editor Mode
- **What:** A mode for painting region areas onto the map. The help describes them as defining where enemy encounter groups appear.
- **Behaviour:**
  - You enter the mode by choosing the [R] tab on the Tile Palette.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_03.html

### RM-EDT-027 — Draw menu: Pencil
- **What:** Paints the selected tile or tile block where you click.
- **Behaviour:**
  - Available from the toolbar or the [Draw] menu. In the 01_04 page, the drawing-tool section is headed [Mode], which looks like a labelling slip.
  - The Draw menu has five tools in total: Pencil, Rectangle, Ellipse, Flood Fill, Shadow Pen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-028 — Draw menu: Rectangle
- **What:** Fills a rectangle with the selected tile.
- **Behaviour:**
  - You drag diagonally from one corner to the opposite corner.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-029 — Draw menu: Ellipse
- **What:** Fills an ellipse with the selected tile.
- **Behaviour:**
  - The ellipse is fitted inside the box you drag out diagonally.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-030 — Draw menu: Flood Fill
- **What:** Replaces the clicked tile and every connected tile of the same kind with the selected tile.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-031 — Draw menu: Shadow Pen
- **What:** Adds or removes wall shadows in quarter-tile steps.
- **Behaviour:**
  - Available from the toolbar or [Draw] > [Shadow Pen].
  - Clicking an unshadowed quarter shades it, and clicking a shaded quarter clears it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-032 — Eyedropper (right-click pick)
- **What:** Picks up existing tiles from the map for painting.
- **Behaviour:**
  - Right-clicking a tile in the Map View picks it up, and a left-click paints it.
  - Right-click-dragging picks up a rectangular block of tiles.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-033 — Autotile override with Shift
- **What:** Lets you place A-tab autotiles without automatic edge fitting.
- **Behaviour:**
  - Holding Shift while drawing or eyedropping temporarily turns off autotile edge fitting.
  - Autotiles are assigned to the A1–A4 slots of the tileset in the Database.
  - Some autotiles add a shadow automatically at the lower right when two or more are stacked vertically. Some tiles never receive shadows.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-034 — Erasing tiles
- **What:** The help gives no dedicated eraser tool.
- **Behaviour:**
  - To erase, you paint with the transparent tile in the upper-left corner of the B tab.
  - Only that upper-left B tile clears all upper-layer tiles.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-035 — Upper/lower tile stacking rules (summary)
- **What:** Rules for how painted tiles combine.
- **Behaviour:**
  - At most two upper-layer (B–E) tiles stack on a cell. Painting a third pushes out the oldest, unless the new tile matches the second.
  - When a passable and an impassable upper tile are stacked, the one placed last decides passability. A ☆ (above-character) tile placed underneath still draws on top. Layering a ☆ tile defers passability to the impassable tile.
  - In A2, columns 1–4 are base tiles and columns 5–8 are decoration tiles placed over a base.
  - In Field-type tilesets, placing a decoration on a 2nd- or 4th-column base changes that base to the 1st- or 3rd-column variant.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html

### RM-EDT-036 — Layer menu
- **What:** A menu equivalent of the Layer button (RM-EDT-008).
- **Behaviour:**
  - Selects the layer to edit, so the other layers are not affected. The 01_04 page lists no sub-items.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-037 — Scale > Zoom In
- **What:** Magnifies the Map View.
- **Behaviour:**
  - Each use zooms in one more step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-038 — Scale > Zoom Out
- **What:** Shrinks the Map View.
- **Behaviour:**
  - Each use zooms out one more step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-039 — Scale > Actual Size
- **What:** Returns the Map View to 1:1 display.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-040 — Tool > Database
- **What:** Opens the Database window, where you create and edit characters, items and other game data.
- **Behaviour:**
  - The [Tool] menu holds: Database, Plugin Manager (RM-PLG-002), Sound Test (RM-AID-001), Event Searcher (RM-AID-004), Character Generator (RM-AID-005) and Resource Manager (RM-AID-016).
  - In 01_04 the menu is headed in the singular, [Tool].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-041 — Game > Playtest
- **What:** The menu entry that starts a test play (details in RM-TST-001).
- **Behaviour:**
  - The [Game] menu holds: Playtest, Open Folder, Update Core Script.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-042 — Game > Open Folder
- **What:** Opens the project's folder in the OS file browser.
- **Behaviour:**
  - The help suggests it for checking where the project is stored or for moving files by hand.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-043 — Game > Update Core Script
- **What:** Changes the version of the runtime core script bundled in the project.
- **Behaviour:**
  - The "Improvements" page describes this as editor-side version management that can upgrade or downgrade the core script.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-EDT-044 — Help > About
- **What:** Per 01_04, this entry brings up the Help window.
- **Behaviour:**
  - The [Help] menu holds: About, RPG Maker Website, Tutorials.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-045 — Help > RPG Maker Website
- **What:** Opens the official RPG Maker website in a browser.
- **Behaviour:**
  - The page links to https://tkool.jp/ for this entry.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-EDT-046 — Help > Tutorials
- **What:** Starts a built-in tutorial (see RM-AID-021).
- **Behaviour:**
  - 01_04 places Tutorials under [Help]. 01_05 gives the path as Tools > Tutorials > Select Tutorial. The two pages disagree.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-EDT-047 — Options: Transparent Color
- **What:** Editor preference for the colour used to show transparent areas in the editor.
- **Behaviour:**
  - 01_05 gives the path as Tools > Options. 01_04's [Tool] menu does not list Options.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-EDT-048 — Options: Map Grid
- **What:** A toggle that draws a grid in the Map Editor.
- **Behaviour:**
  - Ticking "Show" turns the grid on.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-EDT-049 — Options: UI Theme
- **What:** Changes the editor's visual theme.
- **Behaviour:**
  - The help does not list the available themes.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-EDT-050 — Options: UI Object Selector
- **What:** Controls how event-command item pickers and weapon/armor pickers are presented.
- **Behaviour:**
  - Dropdown: a plain drop-down list.
  - Extended: a separate selection window.
  - Smart: chooses between the two automatically, based on how many objects there are.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-EDT-051 — Editor host requirements (help page)
- **What:** The minimum environment needed to run the editor.
- **Behaviour:**
  - OS: Windows 8.1 or 10 (64-bit), or macOS 10.13 or later.
  - CPU: Intel Core i3-4340 (4th generation, 2013) or equivalent, or better.
  - RAM: 8 GB or more.
  - Storage: 8 GB or more of free HDD space.
  - GPU: OpenGL-capable, with 1 GB or more of VRAM.
  - Display: 1280×768 or higher.
  - Network: an internet connection is needed for licence authentication.
  - How smoothly a game runs depends on its content.
  - No separate per-platform runtime requirements appear on this page.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_02.html

### RM-EDT-052 — Editor host requirements (product page differences)
- **What:** The official store/product page lists requirements for each sales channel.
- **Behaviour:**
  - Windows (standalone and Steam) and Mac (Steam only).
  - Storage is listed as **4 GB**, while the help page says 8 GB.
  - The other values match the help page.
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-mz

---

### AID: Aid tools

### RM-AID-001 — Sound Test: purpose and tabs
- **What:** Previews the project's music and sound files (Tool > Sound Test).
- **Behaviour:**
  - Tabs: BGM, BGS, ME, SE.
  - Each tab has a File List, and clicking a track selects it.
  - Playback continues after the window is closed, so it can serve as background music while you work.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-AID-002 — Sound Test: transport controls
- **What:** Starts and stops preview playback.
- **Behaviour:**
  - The [Play] button starts playback, and the [Stop] button halts it.
  - The Space bar also plays the selected track.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-003 — Sound Test: sliders
- **What:** Adjusts how the preview is played.
- **Behaviour:**
  - Playback Volume: 0–100 %.
  - Pitch: 50–150 %.
  - Balance (pan): −100 to 100.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-004 — Event Searcher
- **What:** Finds map events by the switches or variables they use, or by name (Tool > Event Searcher).
- **Behaviour:**
  - Matching events are shown in a result list.
  - The help gives no further search fields or result-list interactions.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-AID-005 — Character Generator: purpose and body-type tabs
- **What:** Builds actor and event graphics from pre-made parts (Tool > Character Generator).
- **Behaviour:**
  - Outputs: face graphics, walking (map) characters and battlers.
  - Base tabs: Male, Female, Kid.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-AID-006 — Character Generator: per-part variation, colour, offset and preview
- **What:** Each feature or part has its own adjustable settings.
- **Behaviour:**
  - Each part has a variation, a colour and an offset.
  - The Preview area updates to show the current choices.
  - Offset (blog): nudges a part up, down, left or right. Examples: moving facial features closer together, seating glasses on the nose, fitting a hat to the hair.
  - Parts named on the product page include hair, eyes, clothing and accessories.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-AID-007 — Character Generator: Randomize
- **What:** Sets every part to a random choice.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-008 — Character Generator: output buttons
- **What:** Each button opens an export window for one image type.
- **Behaviour:**
  - [Face Image]: face graphic.
  - [Walking Character]: map sprite sheet.
  - [Damaged Character]: damaged-pose sheet.
  - [Battler]: side-view battler.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-009 — Character Generator: Export Window
- **What:** Writes the generated image to a file.
- **Behaviour:**
  - [Import]: loads an existing base image from a file, and the generated image is inserted into it (adding to an existing sheet).
  - [Export]: saves the image to a file.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-010 — Character Generator: Save Settings / Load Settings
- **What:** Saves the current character's part choices and loads them back later.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-011 — Character Generator: extensible colour gradients and palettes
- **What:** Users can add their own recolour options.
- **Behaviour:**
  - The blog says there is no fixed limit on gradients for eyes, clothes, accessories, hair and skin.
  - To add a gradient, you add a row to the gradient PNG files, and the generator then shows it as a new choice.
  - The help and product page refer to this as a colour palette feature.
- **Source:** https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-AID-012 — Generate Dungeon: access and map size
- **What:** Draws a procedurally generated, maze-like layout onto the selected map.
- **Behaviour:**
  - You open it by right-clicking the map's name in the Map List and choosing Generate Dungeon.
  - The layout fills the current map. Bigger maps give bigger dungeons, and a very small map will not look much like a dungeon.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-013 — Generate Dungeon: Type
- **What:** Chooses the layout style.
- **Behaviour:**
  - Rooms: rooms joined by corridors.
  - Maze: corridors only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-014 — Generate Dungeon: Options
- **What:** Modifiers for the generated layout.
- **Behaviour:**
  - Add Margins: leaves a border on all four sides of the map.
  - Wide Passages: makes the corridors wider.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-015 — Generate Dungeon: Wall and Floor tiles
- **What:** Picks the tiles used for the generated walls and floors.
- **Behaviour:**
  - Wall: chosen from the A4 (Wall) part of the current tileset.
  - Floor: chosen from the A5 (Floor) part of the current tileset.
  - Once both are chosen, the dungeon is drawn automatically.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-016 — Resource Manager: layout
- **What:** A browser for the project's asset files (Tool > Resource Manager).
- **Behaviour:**
  - Folder List on the left, which also sets the target folder for imports.
  - File List showing the files in the selected folder.
  - Preview, for images only. Audio is previewed in Sound Test instead.
  - The help says custom files must meet the official asset standards (01_11_01; see RM-ASV-004 onward).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-017 — Resource Manager: Import
- **What:** Copies outside files into the selected project folder.
- **Behaviour:**
  - You choose the folder first. Several files can be imported in one go.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-018 — Resource Manager: Export
- **What:** Saves a copy of a project file to a location outside the project.
- **Behaviour:**
  - The original stays in the project.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-019 — Resource Manager: Delete
- **What:** Removes files from the project.
- **Behaviour:**
  - Shift allows several files to be selected.
  - Deletion is permanent and cannot be undone.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-020 — Resource Manager: DLC
- **What:** Imports content from the DLC library.
- **Behaviour:**
  - The DLC button opens the DLC folder and lists the files that apply to the selected folder.
  - The DLC folder sits next to the RPG Maker MZ executable.
  - On Steam, DLC you buy is added to that folder automatically.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-AID-021 — Tutorials
- **What:** Guided, interactive lessons inside the editor.
- **Behaviour:**
  - Path per 01_05: Tools > Tutorials > Select Tutorial.
  - You pick a step from the list and start it, then follow the on-screen prompts.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

---

### TST: Test play, debug tools, battle test, runtime controls

### RM-TST-001 — Playtest
- **What:** Runs the game from the editor at any point during development, so you can check settings and events.
- **Behaviour:**
  - Started from Game > Playtest.
  - The product page notes that Test Play removes the need to deploy for every check.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-TST-002 — Walk through obstacles
- **What:** A playtest-only cheat for moving freely around the map.
- **Behaviour:**
  - Holding Ctrl while moving lets the player pass through tiles that would normally block.
  - 01_06 lists the same thing as Ctrl + Arrow key, playtest only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-003 — Debug Screen (F9)
- **What:** An in-game panel for viewing and editing switches and variables during a playtest.
- **Behaviour:**
  - F9 opens it, and F9 or the cancel button returns to the game.
  - The left list shows ranges of switches (S) at the top and variables (V) below, grouped by number range.
  - The action button moves the cursor into the right-hand list for the chosen range.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-004 — Debug Screen editing
- **What:** How values are changed on the Debug Screen.
- **Behaviour:**
  - Switches: the action button toggles ON/OFF.
  - Variables: left/right, or the L/R buttons, raise and lower the value.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-TST-005 — Developer Tools (F8)
- **What:** Opens browser-style developer tools during a playtest.
- **Behaviour:**
  - They behave like Chrome's built-in developer tools and let you inspect the game's HTML5 elements.
  - Playtest only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-006 — Quitting a playtest
- **What:** Ends a test session.
- **Behaviour:**
  - Close the game window with its close (X) button.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-TST-007 — Battle Test (Troops)
- **What:** Runs a test battle against the selected troop, straight from the Database.
- **Behaviour:**
  - It is listed under the troop's Basic Settings.
  - A setup window has tabs [1] to [4] for choosing the participating actors, their equipment and their levels.
  - A [Status] area shows the resulting parameter values.
  - [OK] opens a battle window and starts the fight. Closing that window ends the test.
  - Plugin settings only take effect in the test after the edited data has been saved.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TST-008 — Special keys at runtime
- **What:** Function keys available while the game is running.
- **Behaviour:**
  - F2: shows or hides the FPS counter and drawing-mode readout.
  - F3: toggles Stretch Mode.
  - F4: toggles full-screen.
  - F5: reloads the game.
  - F8: developer tools (playtest only).
  - F9: Debug Screen (playtest only).
  - Ctrl + arrow: pass through obstacles (playtest only).
  - Mouse wheel: scrolls pages.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-009 — Control scheme: basis
- **What:** The default input layout assumes a four-face-button gamepad, with buttons named after the Xbox 360 controller.
- **Behaviour:**
  - Input methods supported: gamepad, keyboard, mouse and touch.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-010 — Control: Move
- **What:** Moves the player character.
- **Behaviour:**
  - Gamepad: D-pad.
  - Keyboard: arrow keys or numeric keypad.
  - Mouse: click the destination.
  - Touch: tap the destination.
  - Holding the click or tap keeps the player walking in that direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-011 — Control: OK
- **What:** Confirms a choice or interacts with something.
- **Behaviour:**
  - Gamepad: A.
  - Keyboard: Z, Enter or Space.
  - Mouse: click the target.
  - Touch: tap the target.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-012 — Control: Cancel
- **What:** Backs out of the current screen or choice.
- **Behaviour:**
  - Gamepad: B.
  - Keyboard: X, Esc or keypad 0.
  - Mouse: right-click.
  - Touch: two-finger tap, or the on-screen Cancel button.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-013 — Control: Menu
- **What:** Opens the main menu.
- **Behaviour:**
  - Gamepad: Y.
  - Keyboard: X, Esc or keypad 0 (the same keys as Cancel).
  - Mouse: right-click.
  - Touch: two-finger tap, or the on-screen Menu button.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-014 — Control: Dash
- **What:** Moves faster while held.
- **Behaviour:**
  - Gamepad: X.
  - Keyboard: Shift.
  - No mouse or touch binding is listed.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-015 — Control: Previous/Next page
- **What:** Switches between pages, such as different actors in menu screens.
- **Behaviour:**
  - Previous: LB, or Q / Page Up.
  - Next: RB, or W / Page Down.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-016 — Control: Scrolling lists
- **What:** Scrolls through long windows.
- **Behaviour:**
  - D-pad or arrow keys.
  - Mouse: drag from inside the window to outside it, or use the scroll wheel.
  - Touch: press and hold, then swipe from inside the window outward.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-017 — Title menu
- **What:** The commands on the title screen.
- **Behaviour:**
  - New Game: starts a new game from the beginning.
  - Continue: resumes from a save the player picks.
  - Options: general settings, BGM volume for example.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-018 — Field (main) menu
- **What:** The menu opened while walking around.
- **Behaviour:**
  - Opened with the Menu input (RM-TST-013).
  - Commands:
    - Items: use party items, with target actor selection where needed.
    - Skills: use skills, with target selection where needed.
    - Equip: pick an actor, then a slot, then an item. An empty entry removes the equipment.
    - Status: view an actor's details.
    - Formation: reorder the party.
    - Options: general settings.
    - Save: write to one of **20** save slots.
    - End Game: offers To Title or Cancel.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-019 — Battle menu
- **What:** Commands available in combat.
- **Behaviour:**
  - Party Commands, shown each turn:
    - Fight: proceed with the battle.
    - Escape: try to flee. If the escape happens while enemies are already acting, only the enemies act that turn.
  - Actor Commands, shown after Fight:
    - Attack: hit with the equipped weapon.
    - Guard: defend to take reduced damage.
    - Item: use an item.
    - Other commands depend on each actor's setup.
  - The game is over when every party member's HP reaches 0.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-020 — Shop: Buy
- **What:** Buying goods in a shop.
- **Behaviour:**
  - Lists the items, weapons and armor for sale. You pick one, set a quantity and confirm.
  - A side window shows how many you already own.
  - Party members who cannot equip the selected gear are greyed out.
  - With five or more members, the Dash input, or clicking/tapping the side window, pages to the members beyond the fifth.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-021 — Shop: Sell
- **What:** Selling goods in a shop.
- **Behaviour:**
  - You choose a category (Item, Weapon, Armor or Key Item), then an entry and a quantity, and confirm.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-TST-022 — Quantity entry controls
- **What:** The number picker used in shops.
- **Behaviour:**
  - Right: +1. Left: −1.
  - Up: +10. Down: −10.
  - OK confirms.
  - Touch devices get on-screen +1, −1, +10, −10 and OK buttons.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

---

### PLG: Plugins (manager, parameters, commands, annotations, official plugins, runtime API)

### RM-PLG-001 — Plugin system overview
- **What:** Plugins are JavaScript files that add or change engine behaviour, beyond what event commands can do.
- **Behaviour:**
  - Users can write their own plugins or install third-party ones.
  - MZ ships several official plugins (RM-PLG-035).
  - Plugin and command names are picked from drop-downs, and parameters are entered through a simple editor.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-PLG-002 — Plugin Manager window
- **What:** Manages both official and user plugins (Tool > Plugin Manager, or the puzzle-piece toolbar icon).
- **Behaviour:**
  - The plugin list has a Name column.
  - Clicking or double-clicking a row opens that plugin's settings dialog, where it can be turned on or off and its parameter values edited.
  - Parameters appear on the right side of the dialog (blog).
  - The edit screen shows the author and a URL to the author's site (blog, product page).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1 ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command

### RM-PLG-003 — Adding a plugin to the list
- **What:** Registers a plugin file in the project's active list.
- **Behaviour:**
  - You double-click an empty row in the Plugin Manager, choose the plugin from the Name drop-down (which lists the files in the plugin folder) and confirm with OK.
  - The blog adds that you set Status to ON before applying.
  - The plugin then appears in the list.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1

### RM-PLG-004 — Installing third-party plugins
- **What:** Plugin files are placed in a fixed project folder.
- **Behaviour:**
  - Copy the `.js` file into the project's `js/plugins` folder. It then becomes selectable under Name.
  - The blog adds that a new MZ project already contains several plugins in `js/plugins`. A plugin downloaded as plain text must be saved with a `.js` extension.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1

### RM-PLG-005 — Plugin ON/OFF
- **What:** Turns a plugin on or off without removing it from the list.
- **Behaviour:**
  - A check button on each list row toggles the plugin, with no need to open the edit dialog (blog).
  - The blog adds that Space toggles the Status checkbox.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1

### RM-PLG-006 — Refreshing plugins (Update)
- **What:** Re-reads plugin files after they have changed on disk.
- **Behaviour:**
  - After a plugin's `.js` file is replaced, choosing Update picks up its changed parameter definitions and descriptions.
  - Shift-select allows several plugins to be updated at once.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html

### RM-PLG-007 — Parameter value editor
- **What:** The editor for an individual parameter's value.
- **Behaviour:**
  - Clicking a parameter opens an editing panel with a **File** tab and a **Text** tab.
  - Any parameter can be edited as raw text (blog). This makes copy-paste possible, since you can otherwise only copy a whole plugin's settings, not a single parameter.
  - Value kinds the blog mentions: boolean, number, text, and file selection (picking an image such as a picture, character or face from the project folders).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://www.rpgmakerweb.com/blog/beginners-guide-to-plugins-part1

### RM-PLG-008 — Insert Icon Number in parameter fields
- **What:** A helper for entering icon indices.
- **Behaviour:**
  - Right-clicking a numeric or text value offers Insert Icon Number, which opens the IconSet Viewer.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-PLG-009 — Plugin search
- **What:** Search within the Plugin Manager.
- **Behaviour:**
  - In the plugin list, the right-click Search option or Ctrl+F (Cmd+F on Mac) searches the list.
  - In the plugin settings window, Ctrl+F (Cmd+F) searches the plugin's help text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-PLG-010 — Dependency and load-order warnings
- **What:** The manager checks plugin requirements and order.
- **Behaviour:**
  - Plugin authors can declare required plugins and ordering (RM-PLG-020).
  - The plugin list warns when a requirement is missing or the load order is wrong.
- **Source:** https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-PLG-011 — MV plugin notice
- **What:** Flags MV-era plugins.
- **Behaviour:**
  - When a converted project contains MV plugins, the Plugin Manager window shows a message about them.
  - MV plugins may not work in MZ.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-PLG-012 — Plugin Command event command
- **What:** An event command that calls a plugin-defined command.
- **Behaviour:**
  - Plugin Name: chosen from the plugins enabled in the Plugin Manager.
  - Command Name: chosen from that plugin's commands.
  - Arguments: values for the arguments the plugin declares.
  - This replaces MV's single free-text box (blog).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-PLG-013 — Plugin Help from the Script command
- **What:** Access to plugin documentation while scripting.
- **Behaviour:**
  - In the Script event command's text field, the right-click menu includes [Plugin Help], which displays a plugin's help text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html

### RM-PLG-014 — Annotations: whole-plugin header
- **What:** Metadata tags in the plugin file's header comment that the editor reads.
- **Behaviour:**
  - `@target`: always MZ. It separates MZ plugins from MV ones.
  - `@plugindesc`: the title shown in the manager list.
  - `@author`: the author shown in the manager.
  - `@help`: the long usage text shown in the manager.
  - `@url`: the distribution link shown in the manager.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-015 — Annotations: parameters
- **What:** Tags that declare user-editable parameters.
- **Behaviour:**
  - `@param`: the internal name the code reads. It starts each parameter block.
  - `@text`: the display label.
  - `@desc`: the description shown in the input screen.
  - `@default`: the value filled in when the plugin is first turned on. Clearing a value later does not bring the default back.
  - `@type`: the data type, which changes the input dialog.
  - `@parent`: nests a parameter under another, giving a tree.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-016 — Parameter @type values
- **What:** The input widgets available to plugin parameters.
- **Behaviour:**
  - `string`: single-line text.
  - `multiline_string`: multi-line text.
  - `file`: a file picker. Stores the name without its extension, and protects the file from unused-asset pruning.
  - `number`: numeric only.
  - `boolean`: two radio buttons, storing true or false.
  - `select`: a drop-down. Stores the chosen entry's value, or its label if no value is given.
  - `combo`: an editable combo box. Stores the label.
  - Database pickers storing an ID: `actor`, `class`, `skill`, `item`, `weapon`, `armor`, `enemy`, `troop`, `state`, `animation`, `tileset`, `common_event`.
  - `map`: map picker, storing the map ID.
  - `location`: map plus tile picker, storing map ID, X and Y.
  - `switch`, `variable`: pickers storing an ID.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-017 — Type-specific annotation modifiers
- **What:** Extra tags that refine particular types.
- **Behaviour:**
  - `@min` / `@max`: numeric bounds.
  - `@decimals`: number of decimal places (number).
  - `@dir`: the folder the file dialog starts in (file).
  - `@on` / `@off`: the values stored for the two boolean choices.
  - `@option`: a label for each select or combo entry.
  - `@value`: the stored value for a select entry.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-018 — Arrays and structs
- **What:** Composite parameter types.
- **Behaviour:**
  - Adding a trailing `[]` to any type makes it a list, edited in a multi-entry dialog.
  - `struct<Name>` groups several fields under one parameter. The fields are declared in a separate struct block, and the name may not contain symbols.
  - Structs can also be made into arrays.
  - The official plugins are pointed to as worked examples.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-019 — Annotations: plugin commands
- **What:** Tags that declare commands for the Plugin Command event command.
- **Behaviour:**
  - `@command`: the command identifier. It starts each command block.
  - `@arg`: an argument name. It starts each argument block.
  - Commands and arguments use the same label, description, default and type tags as parameters.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-020 — Annotations: dependencies
- **What:** Tags that declare which plugins this one needs and where it must sit in the order.
- **Behaviour:**
  - `@base`: a required base plugin. The manager warns if it is missing.
  - `@orderAfter`: a plugin that must appear above this one in the list.
  - `@orderBefore`: a plugin that must appear below this one.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-021 — Annotations: unused-asset protection
- **What:** Lets plugins protect the files they need from deployment pruning (RM-DEP-007).
- **Behaviour:**
  - `@requiredAssets`: fixed file paths that must not be pruned. Use it when the plugin loads images directly.
  - For images chosen by the user through a parameter, a `file` parameter with `@dir` is enough.
  - For files named in database or map Note fields:
    - `@noteParam`: the note tag name.
    - `@noteDir`: the folder of the referenced file.
    - `@noteType`: always `file`.
    - `@noteData`: which data uses the note. Options: maps, events, actors, classes, skills, items, weapons, armors, enemies, states or tilesets.
  - The MV-era tag for this purpose has been removed.
  - Caution: a plugin that loads images directly can lose them when Exclude Unused Files is on.
- **Source:** https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-PLG-022 — Official plugin: AltMenuScreen
- **What:** An alternative main-menu layout that lists party members horizontally.
- **Behaviour:**
  - The help documents no parameters or commands.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-023 — Official plugin: AltSaveScreen
- **What:** An alternative save/load screen layout that can show images such as party faces.
- **Behaviour:**
  - The help documents no parameters or commands.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-024 — Official plugin: ButtonPicture
- **What:** Makes a displayed picture clickable, so that clicking it runs a common event.
- **Behaviour:**
  - Plugin command: Button Picture Settings.
  - Use: show the picture first with Show Picture, then run the command.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-025 — Official plugin: EventCommandByCode
- **What:** Runs any event command by giving its numeric code and parameter list directly.
- **Behaviour:**
  - Parameters can use variables and control characters.
  - Parameters can take values outside the normal editor limits.
  - Control characters: `\v[n]` (variable), `\s[n]` (switch), `\ss[n]` (self switch), and `\js[n]` (script; needs TextScriptBase).
  - Limits: multi-line commands and deeply nested commands cannot be expressed. Unexpected values may misbehave.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-026 — Official plugin: ExtraImage
- **What:** Adds extra images to scenes that have optional image slots.
- **Behaviour:**
  - Position, scale, origin and similar settings are configurable.
  - Control characters in its parameters can pull values from variables.
  - Parameter: Image List (images added to all screens).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-027 — Official plugin: ExtraWindow
- **What:** Adds extra windows to scenes that have optional window slots.
- **Behaviour:**
  - Position, font size and open/close animation are configurable.
  - Window text supports control characters and redraws when a referenced value changes.
  - Parameter: Window List.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-028 — Official plugin: MaterialBase
- **What:** A registry of image and audio assets, mainly for use by other plugins.
- **Behaviour:**
  - Registered assets are protected from unused-file pruning at deployment.
  - Parameters: Image Assets List, Audio File Assets List.
  - Control characters return the file name for a registered ID: `mi[...]` for images and `ma[...]` for audio, written with the help's backslash/yen prefix.
  - Plugin commands can swap a registered asset for another and show or play it.
  - Using these in Note fields, plugin commands and parameters requires PluginCommonBase-based plugins.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-PLG-029 — Official plugin: OverpassTile
- **What:** Supports bridges and other overpasses that characters can walk both over and under. It improves on the MV version of the same plugin.
- **Behaviour:**
  - Parameters:
    - Overpass Region / Overpass Terrain Tag: mark the overpass tiles.
    - Overpass Entrance Region / Overpass Entrance Terrain Tag: mark the entrances, such as both ends of a bridge.
  - Passage rules: from an entrance, a character can always step onto the overpass. Moving from the overpass toward an entrance is allowed when on top of the bridge but not from underneath. Elsewhere, normal passage rules apply.
  - Events on different levels neither trigger nor collide with each other.
  - Vehicles are not handled.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-030 — Official plugin: PluginCommonBase
- **What:** A base library for other plugins that extends where control characters work.
- **Behaviour:**
  - Control characters: `\v[n]` (variable value), `\s[n]` (switch state), `\ss[n]` (self-switch state).
  - Contexts where they work: message text and description fields as usual. Note fields, plugin commands and plugin parameters also work, but only for plugins built on this base.
  - Parameter: Invalidate Mask Processing (turns off masking where windows overlap).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-031 — Official plugin: RegionBase
- **What:** Database-style settings attached to regions and terrain tags, broadly following the MV Trinity approach.
- **Behaviour:**
  - Per region or terrain tag you can set: passage rules, ladder, bush, counter, damage floor, common events (three trigger types), a switch set on entering the tile, traits applied on the tile, and notes.
  - Parameters: Region List, Terrain Tag List.
  - The data is exposed on the system data object, indexed by region or tag ID. It is empty when unset or when the index is 0.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-032 — Official plugin: TextPicture
- **What:** Renders text as a picture.
- **Behaviour:**
  - Plugin command: Text Picture Settings.
  - Use: run the command, then run Show Picture with no image chosen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-033 — Official plugin: TextScriptBase
- **What:** A registry of reusable multi-line texts and scripts that can be called from control codes.
- **Behaviour:**
  - `tx[id]` inserts a registered text.
  - `js[id]` runs a registered script and inserts its result. It can pass arguments, which are type-converted automatically.
  - A bare `js` form runs an inline script.
  - Parameter: Text Base List.
  - In scripts, `<` and `>` must be written as HTML entities.
  - Note-field, command and parameter use requires PluginCommonBase-based plugins.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-034 — Official plugin: UniqueDataLoading
- **What:** Loads extra, user-defined JSON files from the `data` folder at game start and exposes them as global data.
- **Behaviour:**
  - Parameters:
    - Global Variable Name: the name of the holder object.
    - Data List: the JSON files to load.
  - A boot-scene hook (onUniqueDataLoad) runs once everything has loaded, and can be redefined.
  - If the name includes "window", each file becomes its own global, so watch for name clashes.
  - Files must be valid JSON.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-PLG-035 — Official plugin distribution
- **What:** Where the official plugins come from.
- **Behaviour:**
  - Some official plugins are pre-installed in every new project and work with simple settings. Others are in the separate DLC folder.
  - The help page documents 13 plugins: AltMenuScreen, AltSaveScreen, ButtonPicture, EventCommandByCode, ExtraImage, ExtraWindow, MaterialBase, OverpassTile, PluginCommonBase, RegionBase, TextPicture, TextScriptBase, UniqueDataLoading.
  - The help also says some official plugins support touch-focused games (01_01_01_06), but does not say which ones.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html

### RM-PLG-036 — Runtime architecture: Core API class index
- **What:** The official Core API reference (generated by JSDoc 3.6.5) lists the low-level runtime classes.
- **Behaviour:**
  - Classes listed: Graphics, Input, JsExtensions, JsonEx, TouchInput, Utils, Video, Bitmap, ColorFilter, Point, Rectangle, ScreenSprite, Sprite, Stage, Tilemap, TilingSprite, Weather, WebAudio, Window, WindowLayer.
  - The landing page has no prose on the plugin system, parameters, annotations or plugin commands.
  - It does not describe higher layers such as scenes, managers or game objects.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/

### RM-PLG-037 — Runtime: Graphics (renderer and game loop)
- **What:** A static class in charge of rendering and the main loop.
- **Behaviour:**
  - Exposes the underlying PIXI.Application and the Effekseer context used for particle effects.
  - Holds the screen width and height, the UI "box" width and height, and the default scale.
  - Can register a per-tick handler, start and stop the game loop, and choose which stage is rendered.
  - Also has loading-screen, error-print and retry-button helpers (names listed on the landing page).
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Graphics.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/

### RM-PLG-038 — Runtime: Input and TouchInput
- **What:** Static classes for keyboard/gamepad input and for mouse/touch input.
- **Behaviour:**
  - Input:
    - A key-code map and a gamepad-button map translate raw input into named actions.
    - Repeat wait and repeat interval are measured in frames. No defaults are stated.
    - 4-way and 8-way directions use numpad numbering, with 0 for neutral.
    - Queries: pressed, triggered, repeated, long-pressed.
  - TouchInput has similar queries, plus clicked, hovered, moved, released and cancelled, wheel deltas, and a move threshold (names from the index).
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Input.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/

### RM-PLG-039 — Runtime: Utils (environment, version, encryption)
- **What:** A static utility class.
- **Behaviour:**
  - The engine name constant is "MZ". There is also a version constant and a check for a minimum version (x.x.x format).
  - Option-in-URL check (whether a named option is in the query string).
  - Platform checks: NW.js, mobile, local file, Android Chrome, mobile Safari.
  - Capability checks: WebGL, IndexedDB, Ogg, WebM, CSS font loading, Web Audio.
  - An Arabic-text detector.
  - Encryption flags for images and audio, a decrypt routine, and a setter for the encryption key and flags.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-PLG-040 — Runtime: display classes
- **What:** The rendering building blocks, built on PIXI containers.
- **Behaviour:**
  - Stage: the root of the display tree.
  - Bitmap: an image surface. Loads from a URL, can snapshot the stage, draws outlined text, block transfers, gradients and circles, and has font face/size and outline properties.
  - Sprite: draws a bitmap. Opacity 0–255, hue −360 to 360, blend colour and colour tone as RGBA/RGB-gray, blend mode, and frame rectangle.
  - Tilemap: a tile map display. It takes tileset bitmaps and map size plus data, with optional horizontal/vertical wrap and an autotile animation counter.
  - Window: a game window with skin, contents bitmap, openness 0–255, padding and margin, opacities 0–255, and a tone of −255 to 255 per channel. It also has frame, cursor and pause-sign toggles.
  - WindowLayer: listed as a container for windows.
  - Other display classes listed: TilingSprite, ScreenSprite, Weather, ColorFilter.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Stage.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Bitmap.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Sprite.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Tilemap.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Window.html

### RM-PLG-041 — Runtime: audio and video
- **What:** Media playback classes.
- **Behaviour:**
  - WebAudio: a Web Audio API sound loaded from a URL.
    - Properties: volume, pitch and pan.
    - A global master volume from 0 to 1.
    - Fade-in and fade-out, timed in seconds.
    - Play with a loop flag and a start offset in seconds.
  - Video: a static class with play, playing-state check, volume and resize.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/WebAudio.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/

---

### DEP: Deployment and output formats

### RM-DEP-001 — File > Deployment
- **What:** Packages the project for a chosen platform.
- **Behaviour:**
  - Run from the File menu.
  - The help has a screenshot of the dialog, but its controls are not described in the text (see Not verified).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-002 — Output target: Windows
- **What:** Produces a game folder for Windows.
- **Behaviour:**
  - You distribute the exported folder as it is.
  - Players start the game by double-clicking `Game.exe` (or `Game`) inside it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DEP-003 — Output target: macOS
- **What:** Produces a game folder that contains `Game.app`.
- **Behaviour:**
  - You can distribute either the folder or `Game.app` alone.
  - On Windows, `Game.app` looks like a folder. On macOS it runs as an application.
  - Players launch it by double-clicking `Game.app`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DEP-004 — Output target: Web browsers
- **What:** Produces a folder to upload to a web server, so the game can be played in a browser.
- **Behaviour:**
  - No native Android or iOS app packaging is described. Mobile play is through the browser.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-005 — Web play requirements
- **What:** What a browser or device needs to run a web build.
- **Behaviour:**
  - HTML5 Canvas and the Web Audio API are required.
  - iOS 12 or later. Android 8 or later.
  - WebGL is required.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-006 — Recommended browsers and cautions
- **What:** The browsers the help recommends for web builds.
- **Behaviour:**
  - Google Chrome, Safari, Firefox, iOS Safari, Chrome for Android.
  - Smartphone players are advised to use Wi-Fi.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-007 — Option: Exclude Unused Files
- **What:** Leaves unreferenced assets out of the package to make it smaller.
- **Behaviour:**
  - Candidates for removal: everything under the project's `img`, `audio` and `effects` folders, including subfolders.
  - Files used only by plugins may be dropped, depending on how the plugin is written. Plugins can protect them with annotations (RM-PLG-021).
  - Users can protect specific files by registering them in the official MaterialBase plugin.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-DEP-008 — Encryption: Image Files
- **What:** A checkbox that encrypts every image in the package.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-009 — Encryption: Sound Files
- **What:** A checkbox that encrypts every audio file in the package.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html

### RM-DEP-010 — Encryption: Encryption Key and cautions
- **What:** A text key used to encrypt the files.
- **Behaviour:**
  - Encrypted files cannot be used inside the project folder; they are for output only.
  - The help says encryption cannot guarantee protection against copying of browser games, because of factors the developer cannot control.
  - The runtime Utils class carries matching encryption flags, a key setter and a decrypt routine (RM-PLG-039).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-DEP-011 — Editor sales channels (not output targets)
- **What:** The product page lists where the editor itself is sold.
- **Behaviour:**
  - Windows: standalone and Steam.
  - Mac: Steam only.
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-mz

---

### AST: Asset and side-view standards

### RM-ASV-001 — Side-view actor battler sheet
- **What:** The image format for actor battle graphics in Side-View mode. Each actor has its own file.
- **Behaviour:**
  - One sheet is a 9 × 6 grid, giving 54 cells.
  - The standard sheet size is 576 × 384 px.
  - By our arithmetic each cell is 64 × 64 px; the page does not state this.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html

### RM-ASV-002 — Side-view motion = three frames in a row
- **What:** How the frames on the sheet are grouped into motions.
- **Behaviour:**
  - Three horizontally adjacent cells make one motion.
  - The page does not name the motions or give their order on the sheet.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html

### RM-ASV-003 — Side-view frame playback order
- **What:** How a motion's three frames are played.
- **Behaviour:**
  - Looping idle (standby) motions cycle left to right through frames 1, 2, 3, 2.
  - One-shot motions, such as damage, play 1, 2, 3 once.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html

### RM-ASV-004 — General asset placement rules
- **What:** Custom assets become selectable in the editor once they are copied into the matching project folder.
- **Behaviour:**
  - Subfolders are allowed.
  - Folder names should use half-width alphanumeric characters.
  - Enemy and picture images with odd widths can look blurry, so even widths are recommended.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-005 — Image folders and sizes (PNG)
- **What:** The image asset folders and their expected sizes.
- **Behaviour:**
  - `img/battlebacks1` (floor) and `img/battlebacks2` (wall): 1000 × 740.
  - `img/enemies`: front-view enemies, any size.
  - `img/faces`: 144 × 144 per face, in a 2 × 4 grid of 8.
  - `img/parallaxes`: any size, and can be made to loop seamlessly.
  - `img/pictures`: any size.
  - `img/sv_actors`: see RM-ASV-001.
  - `img/sv_enemies`: any size.
  - `img/titles1` and `img/titles2`: 816 × 624.
  - `img/characters`: 48 × 48 per frame, with 4 directions × 3 patterns per character.
  - `img/system`: `Baloon.png` (spelled this way on the page), `ButtonSet.png`, `GameOver.png`, `IconSet.png`, `Shadow1.png`, `Shadow2.png`, `States.png`, `Weapons1.png`, `Weapons2.png`, `Weapons3.png`, `Window.png`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-006 — Character filename prefixes
- **What:** Special characters at the start of a character sheet's filename change how it is treated.
- **Behaviour:**
  - `$`: the file holds a single character instead of a multi-character sheet.
  - `!`: no 6 px upward offset and no half-transparency in bushes. Meant for objects such as doors and chests.
  - The two prefixes can be combined.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-007 — Parallax filename prefix
- **What:** A filename prefix for parallax images.
- **Behaviour:**
  - A leading `!` makes the parallax act as a fixed floor image, with no parallax scrolling.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-008 — Tileset sheets
- **What:** Layout and sizes of the tileset images in `img/tilesets`.
- **Behaviour:**
  - Tiles are 48 × 48.
  - B–E (upper layers) are 768 × 768, a 16 × 16 grid.
  - A1 (water): 768 × 576.
  - A2 (ground): 768 × 576.
  - A3 (buildings): 768 × 384.
  - A4 (walls): 768 × 720.
  - A5 (plain tiles, 8 × 16): 384 × 768.
  - Behaviour varies with the tileset Mode setting in the Database.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-009 — Audio format and loop points
- **What:** The audio format MZ expects.
- **Behaviour:**
  - Ogg Vorbis (`.ogg`).
  - If the file's metadata contains LOOPSTART and LOOPLENGTH, playback loops on those points.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ASV-010 — Movie formats per platform
- **What:** The video formats MZ expects.
- **Behaviour:**
  - WebM (`.webm`) and MP4 (H.264, `.mp4`).
  - Windows and Mac builds use WebM only.
  - Web, Android and iOS builds need both formats, with the same base filename.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

---

### L10N: Localization

### RM-L10N-001 — Built-in multi-language (game localization) support
- **What:** Not documented in the official help.
- **Behaviour:**
  - None of these official pages describes a game-language setting, string tables, translation files or runtime language switching: 01_01* (all), 01_02 to 01_06, 01_08_12_01 (System 1), 01_08_12_02 (System 2), 01_08_13 (Type Settings), 01_08_14 (Terms Settings), 01_11_02 to 01_11_05, the product page, and the Core API index.
  - The official annotation page does not cover language-specific annotation blocks.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz ; https://rpgmakerofficial.com/product/mz/plugin/make/annotation.html

### RM-L10N-002 — Terms Settings: one set of in-game vocabulary
- **What:** The Database Terms page defines the game's UI words in a single language.
- **Behaviour:**
  - Covers basic status labels (Level, HP, MP, TP, EXP), parameter names, equipment slot names, command names, and runtime message templates with placeholders.
  - There is one set per project. No per-language variants are mentioned.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-L10N-003 — Type Settings: renameable category names
- **What:** Elements, skill types, weapon types, armor types and equipment types are user-named lists.
- **Behaviour:**
  - A [Change Maximum] button resizes each list.
  - Again, there is only one language.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-L10N-004 — Fonts and Fallback Font (System 2 > Advanced Settings)
- **What:** Font settings that matter for character-set coverage.
- **Behaviour:**
  - Main Font Filename and Number Font Filename choose the font files.
  - Fallback Font supplies glyphs that the main font lacks.
  - Font Size sets the text size.
  - Fonts are set here in the database rather than through MV's CSS file.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-L10N-005 — Editor text entry depends on the OS IME
- **What:** How text is typed into editor fields.
- **Behaviour:**
  - Text entry goes through the user's OS input method editor.
  - Full-width digits typed into number fields are converted to half-width automatically.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-L10N-006 — Runtime script-detection hint
- **What:** The only language-related API member found.
- **Behaviour:**
  - The Core API Utils class has a check for whether a string contains Arabic characters.
  - Its purpose is not explained.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

---

### NEW: Features introduced in MZ (relative to MV)

### RM-NEW-001 — Feature overview categories
- **What:** The help groups MZ's changes into five categories.
- **Behaviour:**
  - (1) Creation-environment improvements: easier MV migration, faster work, and fewer input mistakes.
  - (2) Gameplay control improvements: autosave, save transfer by Game ID, menu UI changes, and browser and touch controls as standard alongside the keyboard.
  - (3) A new battle system (TPB).
  - (4) Artistic expression: the animation editor, character generator, control characters, the "Just Prior" operand and Show Text.
  - (5) Other: configurable resolution and font settings, among others.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html

### RM-NEW-002 — Time Progression Battles (TPB)
- **What:** An alternative to turn-based battles in which each battler has a gauge that fills over time.
- **Behaviour:**
  - When an actor's gauge reaches 100 %, they choose an action.
  - After acting, the gauge resets to 0 and starts filling again.
  - The product page says action speed depends on stats and states.
  - The help's TPB page calls the gauge a "TP gauge"; whether this is the same as skill TP is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-003 — Selecting the battle system
- **What:** Where TPB is switched on.
- **Behaviour:**
  - Database > System 1 > Battle System.
  - Front-view and side-view battles both remain available.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-004 — Animation editor rebuilt around Effekseer
- **What:** Battle and field animations are now particle effects made with Effekseer, an open-source tool.
- **Behaviour:**
  - Effects can be 2D or 3D, with no sprite sheets needed.
  - In the editor you choose an effect and attach sound and flash timing to it.
  - Animations are used by items, skills, weapons and the Show Animation event command.
  - Effekseer setup lives under Database > Animations > Effekseer.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_04.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-005 — Character Generator improvements
- **What:** The generator gains part offsets and a colour palette (see RM-AID-006 and RM-AID-011).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-006 — Map layer selection
- **What:** You can edit one layer at a time (see RM-EDT-008).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-007 — "Just Prior" variable operand
- **What:** A new Game Data operand in Control Variables that reads information about the action that just happened.
- **Behaviour:**
  - Example from the help: an enemy says different last words depending on which skill defeated it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-008 — Scope selection dialog for skills and items
- **What:** Skill and item Scope is now set in a dedicated target-selection dialog.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-009 — Multiple "Common Event" effects
- **What:** One skill or item can list several Common Event effects.
- **Behaviour:**
  - All of the listed common events run.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-010 — Attack Skill trait
- **What:** A trait on actors or weapons that sets which skill the Attack command uses.
- **Behaviour:**
  - Lets weapons trigger a skill other than the standard attack.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-011 — Troop screen enemy ID display
- **What:** Selecting a placed enemy in a troop shows its index ID.
- **Behaviour:**
  - Helps when entering target IDs into plugin commands.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-012 — System settings split into System 1 / System 2
- **What:** System settings are spread over two pages and include new options.
- **Behaviour:**
  - The title-screen command window's type and position can be set.
  - Time progression is added to Battle System.
  - New Options entries: Show TP in Window, Show Number of Key Items (System 1 calls it Show Key Item Quantities), Enable Autosave.
  - System 1's Options group also lists Start Transparent, Show Player Followers, Death by Slip Damage, Death by Floor Damage and EXP for Reserve Members.
  - System 2 Advanced Settings: Game ID, Screen Width, Screen Height, UI Area Width, UI Area Height, Main Font Filename, Number Font Filename, Fallback Font, Font Size.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-NEW-013 — Show Text: speaker Name field
- **What:** Show Text gains a Name field that displays the speaker's name above the message box.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-014 — New text control characters PX, PY, FS
- **What:** Message control characters for finer text layout.
- **Behaviour:**
  - The help names PX, PY and FS for finer text positioning and font sizing. Exact semantics are on the Show Text page, outside this slice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-015 — Right-click Insert Color Number / Insert Icon Number
- **What:** Context-menu helpers in text fields.
- **Behaviour:**
  - Right-clicking the message text field offers Insert Color Number and Insert Icon Number.
  - The same Insert Icon Number helper, with the IconSet Viewer, exists in plugin parameter fields (RM-PLG-008).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-NEW-016 — Move Picture easing
- **What:** The Move Picture event command gains an Easing setting.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-017 — Conditional Branch touch and mouse conditions
- **What:** Button conditions in Conditional Branch become more detailed.
- **Behaviour:**
  - Adds input states such as long press.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-018 — Bundled official plugins
- **What:** Several plugins ship with MZ (see RM-PLG-022 to RM-PLG-035).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-NEW-019 — Core script version management
- **What:** Game > Update Core Script can move the project's core script up or down a version (RM-EDT-043).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-020 — Plugin Command improvements
- **What:** Plugin commands are picked from structured drop-downs, with typed arguments (RM-PLG-012).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-021 — Movement route preview
- **What:** Set Movement Route has a preview button that plays the route while you edit it.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-022 — Coordinates shown when picking a Transfer Player destination
- **What:** The location-picker dialog in Transfer Player shows coordinates.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-023 — Show Balloon Icon preview
- **What:** You can preview the balloon animation while configuring the command.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-024 — Full-width digit auto-conversion
- **What:** Full-width digits typed into number fields become half-width automatically.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-NEW-025 — Smartphone and touch-friendly UI
- **What:** Menus are designed to work on both PC and smartphone.
- **Behaviour:**
  - Windows scroll by swiping.
  - Battle targets can be chosen by touch.
  - Official touch-oriented plugins are included.
  - The product page says the UI was redone for mouse and touch.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-026 — Autosave
- **What:** The game saves itself automatically into a reserved slot.
- **Behaviour:**
  - The top save slot is reserved for autosave.
  - It saves after each battle and on every map transfer, so play can resume after an unexpected quit.
  - It can be turned off in Database > System 1 > Options (Enable Autosave).
  - The product page describes the autosave slots as separate from normal saves.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-027 — Game ID
- **What:** A per-project identifier that tells games apart.
- **Behaviour:**
  - For browser builds, save data carries over to an updated version even if the title has changed.
  - It is editable in Database > System 2 > Advanced Settings.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-NEW-028 — Configurable resolution and font settings
- **What:** Screen resolution, UI area and fonts are now project settings (System 2 > Advanced Settings).
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-NEW-029 — Map tree reordering
- **What:** Maps can be rearranged in the map tree, including between siblings at the same level.
- **Behaviour:**
  - None documented beyond the description above.
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-NEW-030 — Larger built-in tile set and pixel-accurate map saving
- **What:** The product page claims many new tiles and pixel-accurate maps saved from the editor.
- **Behaviour:**
  - The page says maps saved from the built-in editor keep exact pixel detail rather than being resized.
  - What "saved" refers to (for example, a map image export) is not explained.
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-mz

---

## 3. Not verified

Items the fetched official pages do not state, or state only in screenshots. Do not implement any of these from memory.

1. Keyboard shortcuts for any menu-bar command. 01_04 lists none.
2. The complete toolbar button list and its order.
3. Whether the Draw menu is actually labelled "Draw". 01_04 heads the drawing-tool section [Mode] a second time, while 01_07_01 says [Draw].
4. Where Options and Tutorials sit in the menus. 01_05 says Tools > Options and Tools > Tutorials. 01_04 lists no Options entry and puts Tutorials under [Help].
5. What Help > About actually shows. 01_04 says it brings up the Help window; a version/about dialog is not described.
6. Search window fields: case sensitivity, scope and match options.
7. Event Searcher: search fields beyond switch, variable and name; result-list columns; jump-to-event behaviour.
8. Character Generator: the list of part names, output image sizes and formats, palette file names and locations, offset step and limits.
9. The Generator export window's exact "Import" semantics beyond loading a base image and inserting into it.
10. Generate Dungeon: whether it overwrites existing tiles or events, and any minimum map size.
11. Resource Manager: the supported import file types.
12. Sound Test: whether ME and SE stop after closing, and whether volume/pitch/pan settings are saved.
13. Debug Screen: the size of each switch/variable range group and how paging works.
14. Developer tools and Debug Screen on macOS (key equivalents).
15. Battle Test: how to open it (button location), and which of tabs [1] to [4] holds which setting.
16. The in-game Options menu entries (for example dash, command memory, touch UI, per-channel volumes). 01_06 says only that it covers settings such as BGM volume.
17. Runtime keys not listed: F11, F12 and others; gamepad buttons beyond the 4-button Xbox-style set.
18. Deployment dialog fields: output folder or location, platform selector UI, and whether NW.js is used.
19. Native Android, iOS or Linux packaging. Only browser play on Android 8+ and iOS 12+ is documented.
20. Side-view battler: motion names and the row/column order of motions; weapon sprite standards; state-overlay standards. 01_11_02 gives only the grid, size and frame order.
21. Plugin Manager: deleting plugins from the list and reordering them (drag or buttons); the full field list of the plugin edit dialog (only Name, Status, parameters, author and URL are evidenced).
22. Plugin annotations not on the official annotation page: `@noteRequire`, a `note` type, language-specific annotation blocks (for example `/*:ja`), and `@require`.
23. Defaults for parameter types (for example the numeric default range when `@min`/`@max` are absent).
24. Core API: no prose on the plugin loader, plugin parameter access, scenes, managers or game objects. Only the low-level class index is documented. Key-repeat defaults are not stated.
25. Editor UI languages offered. The product page lists none, and the help does not cover it.
26. Built-in game localization or multi-language support: **not documented in official help**.
27. The `icon` folder listed in the MV conversion table: its contents and purpose.
28. Storage requirement: the help says 8 GB and the product page says 4 GB. Which is current is not verified.
29. Autosave: whether it is one reserved slot (help) or several separate slots (product page); the exact count.
30. The UI Theme choices in Options.
31. The Transparent Color default value.
32. The undo limit beyond "20 actions": whether it is per map or global, and whether database edits are covered.
33. The meaning of the product page's "pixel-perfect" map-saving claim (RM-NEW-030).
34. The exact syntax and semantics of the PX, PY and FS control characters. These are on the Show Text page, outside this slice.
35. Whether the "TP gauge" named on the TPB page is the same as skill TP.
36. Which official plugins ship in the DLC folder rather than in new projects.
37. The full help table of contents. The index pages load their navigation dynamically, and it could not be captured.
