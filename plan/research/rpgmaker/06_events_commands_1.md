# RPG Maker MZ — Events & Event Commands (part 1): sourced feature inventory

Clean-room research notes for Talewright. All behavior below is paraphrased in our own words from the official RPG Maker MZ help (English). Command and field names are quoted as names only. No code.

## 1. Pages covered

Retrieval date: 2026-10-13. Method: WebFetch on the official URLs returned only short summaries (its summariser would not return the full text), so the full page text of the same official URLs was downloaded directly over HTTPS (all returned HTTP 200) and read in full. The UI screenshots on each page (served from `https://rpgmakerofficial.com/product/MZ_help-en/assets/img/...`) were also viewed. Anything taken only from a screenshot (default values, option labels) is marked **(screenshot)**.

| Page | URL | Status |
|---|---|---|
| Events | https://rpgmakerofficial.com/product/MZ_help-en/01_09.html | Loaded |
| Map Event System | https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html | Loaded |
| Map Event Creation and Controls | https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html | Loaded |
| Map Event Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html | Loaded |
| Event Commands (overview) | https://rpgmakerofficial.com/product/MZ_help-en/01_10.html | Loaded |
| Messages | https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html | Loaded |
| Game Progression | https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html | Loaded |
| Flow Control | https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html | Loaded |
| Party | https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html | Loaded |
| Actor | https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html | Loaded |
| Movement | https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html | Loaded |
| Movement Route Settings (sub-page, linked from 01_09_03 and 01_10_06) | https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html | Loaded |
| Character | https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html | Loaded |
| Picture | https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html | Loaded |

In-help links not followed (outside this slice; they are only cited as places where control characters also work, or as where battle/common events are edited): 01_08_01 (Actors), 01_08_03 (Skills), 01_08_04 (Items), 01_08_05 (Weapons/Armor), 01_08_07 (Troops), 01_08_08 (States), 01_08_11 (Common Events), 01_08_14 (Terms).

Notes on the help itself:
- In the help, "Battle events: not stated" means the command page says nothing about battle-event use either way.
- Help wording that looks like a typo or contradicts a screenshot is flagged inline.

---

## 2. Items

### AREA EVT — Event system, pages, conditions, triggers, priority, quick events, editor

### RM-EVT-001 — Three kinds of events
- **What:** Events are the data that produce in-game happenings (dialog, chests, boss fights and so on) in response to game state and player actions. The help splits them into three kinds.
- **Behavior:**
  - Map Events: run on the map screen. Used for NPC talk, chests, and also for imageless story/progression logic such as moving the player between places. Story progression is generally built with this kind.
  - Battle Events: run on the battle screen, e.g. an enemy changing form once its HP falls under a threshold. Authored in the database's Troops tab.
  - Common Events: shared, generic events that Map Events and Battle Events can call, e.g. the effect of a special item. Authored in the database's Common Events tab.
  - Map Events are edited in event-editing mode (toolbar [Events] button or menu [Mode] → [Events]).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09.html

### RM-EVT-002 — Map event model (where / when / trigger / what)
- **What:** A map event is described by four things: its map location, the situation in which it is present (appearance conditions), the action that starts it (trigger), and what it does (contents).
- **Behavior:**
  - Visible things (people, chests) are made by giving the event an image; invisible logic (doorways, automatic cut-scenes) uses events with no image.
  - Authoring flow: first place the event on the map (location plus contents), then fill in conditions and the rest.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-003 — Event pages (up to 20 per event)
- **What:** One map event can hold several pages, each a separate variant chosen by game state.
- **Behavior:**
  - Maximum of 20 pages per event.
  - Each page has its own image, trigger and contents (and its own conditions, options, priority and movement, per the page settings).
  - In effect a single map event can act as up to 20 different events, depending on conditions.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-004 — Active page selection rule
- **What:** Decides which page of a map event is in effect.
- **Behavior:**
  - Out of all pages whose conditions are currently satisfied, the page with the highest page number wins.
  - If no page satisfies its conditions, the event does nothing on the map (01_09_03 likewise says no event occurs).
  - A page with no conditions enabled always qualifies.
  - When a page enables several conditions, they must all hold (logical AND).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-005 — Switches (global ON/OFF flags)
- **What:** Game-wide boolean flags used to record story progress (e.g. "quest accepted", "boss beaten").
- **Behavior:**
  - Value is ON or OFF; every switch is OFF when a new game starts.
  - Changed with the [Control Switches] command.
  - Several events can use the same switch in their page conditions, so turning one switch ON can change many events at once.
  - Up to 5,000 switches per game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-006 — Variables (global integer storage)
- **What:** Game-wide integer slots for counts and calculations.
- **Behavior:**
  - Integer values; every variable is 0 when a new game starts.
  - Changed with [Control Variables] (assignment or arithmetic with other values).
  - Can hold game data (e.g. player HP, gold owned) so that event conditions and results can depend on it.
  - Up to 5,000 variables per game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-007 — Naming switches and variables
- **What:** Creators can label each switch and variable.
- **Behavior:**
  - The help suggests using names that make each slot's purpose clear.
  - Names are entered in a [Name] field in the switch/variable selection windows.
  - Which number tracks which game state is entirely up to the creator.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-008 — Self Switches (A–D, per event)
- **What:** Four ON/OFF flags owned by one map event, used only by that event.
- **Behavior:**
  - Four per event, named A, B, C, D.
  - Typical use: a chest that tracks "already opened" in a self switch, with a "closed" page and an "opened" page, so its item can only be taken once.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_01.html

### RM-EVT-009 — Event editing mode
- **What:** The editor mode for placing and editing map events.
- **Behavior:**
  - Turned on with the toolbar [Events] button or menu [Mode] → [Events].
  - In this mode the tile palette is replaced by an Event List (see RM-EVT-012).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-010 — Creating a new map event
- **What:** Placing a new event on the map.
- **Behavior:**
  - Double-click an empty spot on the map view in event mode; the [Event Editor] window opens; confirm with [OK].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-011 — Event icon, re-editing and moving
- **What:** How placed events look in the editor and how they are changed.
- **Behavior:**
  - A placed event shows as an icon on the map, drawn with the image of the event's first page.
  - Double-click the icon to open the Event Editor again.
  - Drag and drop the icon to move the event to another tile.
  - Event icons have a white frame (the starting position uses a blue frame).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-012 — Event List panel
- **What:** A list of the map's events that replaces the tile palette in event mode.
- **Behavior:**
  - Selection is synced both ways: picking an entry selects the event on the map, and selecting an event on the map highlights its entry.
  - Double-clicking an entry opens the Event Editor.
  - Entries show ID and name, e.g. "001 EV001" **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-013 — Map view context menu (event mode)
- **What:** Right-click menu on the map view while in event mode.
- **Behavior:**
  - Edit: opens the Event Editor; shown as "New" when the tile has no event yet.
  - Cut: puts the clicked event on the clipboard and removes it from the map.
  - Copy: puts the clicked event on the clipboard.
  - Paste: places the clipboard event at the clicked tile.
  - Delete: removes the clicked event.
  - Quick Event Creation: submenu of four templates (RM-EVT-015 to 018).
  - Set Starting Position: see RM-EVT-014.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-014 — Set Starting Position
- **What:** Sets where the player and the vehicles begin a new game.
- **Behavior:**
  - Chosen from the map context menu at the clicked tile.
  - Shown on the map with a blue frame (events use white frames).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-015 — Quick Event: Transfer ("Movement")
- **What:** A template that makes an invisible event that moves the player somewhere else, for entrances and stairs.
- **Behavior:**
  - The event has no image.
  - Inputs: [Location] (where the player arrives) and [Direction] (which way the player faces on arrival).
  - Like any event, the generated event can be edited and copied afterwards.
  - Naming in the help: the intro lists the four templates as [Transfer], [Door], [Treasure], [Inn], but this template's own heading says "Movement".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-016 — Quick Event: Door
- **What:** A template that makes a door event that moves the player somewhere else.
- **Behavior:**
  - Inputs: [Image] (door graphic) and [Location] (where the player arrives).
  - Can be edited afterwards like any event.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-017 — Quick Event: Treasure
- **What:** A template that makes a chest that gives its contents only the first time it is checked.
- **Behavior:**
  - Inputs: [Image] (chest graphic) and [Content]: one of gold, an item, a weapon or an armor.
  - Gives its contents only once.
  - Can be edited afterwards like any event.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-018 — Quick Event: Inn
- **What:** A template that makes an event which fully heals the party for a fee.
- **Behavior:**
  - Inputs: [Image] and [Price].
  - Fully restores the party.
  - Can be edited afterwards like any event.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html

### RM-EVT-019 — Event ID
- **What:** A unique number for each event on a map.
- **Behavior:**
  - Assigned automatically per map, in order of creation.
  - Shown in the Event Editor title bar (e.g. "ID:001 - Event Editor") **(screenshot)**.
  - Used wherever an event is referenced by number, e.g. through variables.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-020 — Event Name
- **What:** An editor-only label for the event.
- **Behavior:**
  - Has no effect on the game.
  - Pre-filled with a name based on the ID, e.g. "EV001" **(screenshot)**. It can be renamed.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-021 — Event Memo / Note
- **What:** A free-text notes field on the event.
- **Behavior:**
  - The text calls it "Memo" and the screenshot labels it "Note:".
  - Any text can be entered.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-022 — Event page tabs
- **What:** Numbered tabs, one per page, in the Event Editor.
- **Behavior:**
  - Click a tab to edit that page. Page numbers start at 1 **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-023 — Event page control buttons
- **What:** Buttons at the top of the Event Editor that manage pages.
- **Behavior:**
  - New Event Page: adds a page directly after the current one. Each new page's number is one higher.
  - Copy Event Page: puts the current page on the clipboard.
  - Paste Event Page: inserts the clipboard page directly after the current page, with the next number.
  - Delete Event Page: removes the current page. Every later page's number goes down by one.
  - Clear Event Page: resets the current page to its blank initial state.
  - In a fresh single-page event, Paste and Delete are grayed out **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-024 — Appearance Conditions (general)
- **What:** Per-page conditions that decide whether that page can be active.
- **Behavior:**
  - Condition types: Switch, Variable, Self Switch, Item, Actor. Each is turned on with its checkbox and then given a target.
  - With none enabled, the page qualifies unconditionally.
  - With several enabled, all must be true.
  - Page selection is described in RM-EVT-004.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-025 — Page condition: Switch (two slots)
- **What:** The page qualifies when a chosen switch is ON.
- **Behavior:**
  - The text describes one switch condition, but the screenshot shows two separate [Switch] checkboxes/fields **(screenshot)**. When both are enabled, both must be ON (by the all-conditions rule).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-026 — Page condition: Variable
- **What:** The page qualifies when a chosen variable reaches a fixed number.
- **Behavior:**
  - One variable and one constant.
  - The text says "greater than" the value, but the screenshot shows the comparison as "≥" (greater than or equal) **(screenshot)**. Conflict noted; see the Not verified list.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-027 — Page condition: Self Switch
- **What:** The page qualifies when one of this event's self switches (A–D) is ON.
- **Behavior:** Self switch letter is chosen from a dropdown **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-028 — Page condition: Item
- **What:** The page qualifies when the party holds a chosen item.
- **Behavior:** Item is chosen from a dropdown **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-029 — Page condition: Actor
- **What:** The page qualifies when a chosen actor is a party member.
- **Behavior:** Actor is chosen from a dropdown **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-030 — Page Image
- **What:** The graphic used for the event on the map while this page is active.
- **Behavior:**
  - Double-click the image box to open [Select an Image]. Pick a file on the left, then the specific image on the right.
  - With no image, nothing is drawn on the map.
  - Choosing [(None)] at the top of the file list removes the image.
  - The image can be a tile (see Priority, RM-EVT-038).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-031 — Autonomous Movement: Type
- **What:** How the event moves by itself while the page is active.
- **Behavior:**
  - Fixed: does not move. This is the default **(screenshot)**.
  - Random: wanders randomly.
  - Approach: heads toward the player.
  - Custom: follows a route defined with the [Route...] / Set Movement Route editor (see the MRT area). The Route button is grayed out unless Custom is selected **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-032 — Autonomous Movement: Speed
- **What:** How fast the event moves.
- **Behavior:**
  - Higher number = faster.
  - Default shown as "3: x2 Slower" **(screenshot)**. The full list of levels is not given in the text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-033 — Autonomous Movement: Frequency
- **What:** How often the event takes a movement step.
- **Behavior:**
  - Higher number = shorter pause between moves.
  - Default shown as "3: Normal" **(screenshot)**. The full list of levels is not given in the text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-034 — Option: Walking (walking animation)
- **What:** Plays the walk-cycle animation while the event moves.
- **Behavior:**
  - For people and animals that should look like they are walking.
  - Checked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-035 — Option: Stepping (stepping animation)
- **What:** Keeps the walk-cycle animation running even while the event stands still.
- **Behavior:**
  - For things like rippling water or flickering fire.
  - Unchecked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-036 — Option: Direction Fix
- **What:** Keeps the graphic's facing from changing when the event moves.
- **Behavior:** Unchecked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-037 — Option: Through
- **What:** Lets the event pass through terrain and events that would normally block it.
- **Behavior:** Unchecked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-038 — Priority
- **What:** The event's layer relative to the player and other characters. It controls draw order when they overlap, and whether they can share the tile.
- **Behavior:**
  - Below characters: the player and others can walk onto it. If the page image is a tile, passability instead follows that tile's settings.
  - Same as characters: on the same layer as characters and blocks movement into its tile.
  - Above characters: the player and others can walk underneath it.
  - Whichever is on top is drawn in front.
  - Default: Below characters **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-039 — Trigger: Action Button
- **What:** The page's contents run when the player presses the action button at the event.
- **Behavior:**
  - Fires when the player is next to the event, facing it, and presses the button.
  - Also fires when the player is on the same tile, for events that can be overlapped.
  - Default trigger **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-040 — Trigger: Player Touch
- **What:** Contents run when the player makes contact with the event.
- **Behavior:**
  - Fires when the player bumps into the event, or steps onto it if overlapping is possible.
  - Also fires on an action-button press while in that position.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-041 — Trigger: Event Touch
- **What:** Contents run when the event, through its own autonomous movement, reaches the player.
- **Behavior:**
  - Fires when the event's own movement brings it into contact with (or onto, if overlap is possible) the player.
  - Also fires under all the Player Touch conditions.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-042 — Trigger: Autorun
- **What:** Contents run on their own as soon as the page becomes active (its conditions are met).
- **Behavior:** The help says only that it starts when the event appears with its conditions met. Blocking and repeat behavior is not described on this page.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-043 — Trigger: Parallel
- **What:** Contents run in a loop for as long as the page is active.
- **Behavior:**
  - Begins once the page's conditions are met and repeats its contents.
  - Keeps running even while other events are executing (concurrent).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-044 — Contents list (event command list)
- **What:** The ordered list of event commands that runs when the trigger fires.
- **Behavior:**
  - Commands run from top to bottom.
  - Rows starting with "◆" are commands.
  - Rows starting with ":" show a command's extra settings or mark where a branch splits.
  - An empty list contains a single "◆" row **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-045 — Adding event commands
- **What:** Inserting commands into the contents list.
- **Behavior:**
  - Double-click a "◆" row to open the command picker, choose a command, then fill in its settings (some commands have none).
  - Double-clicking a row that already holds a command inserts the new command at that position.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-046 — Contents list context menu
- **What:** Right-click menu on a command row.
- **Behavior:**
  - New: insert a command at the selected row.
  - Edit: reopen the selected command's settings.
  - Cut: copy the row(s) to the clipboard and remove them.
  - Copy: copy the row(s) to the clipboard.
  - Paste: insert the clipboard at the selected row.
  - Delete: remove the selected command.
  - Select All: select the whole list.
  - Copy as Text: copy the selected lines as plain text.
  - Copy as HTML Tags: copy the selected lines as HTML.
  - Test: run the selected commands in a game-screen test.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-047 — Multi-selecting commands
- **What:** Selecting a range of commands to act on all of them at once.
- **Behavior:**
  - Select a row, then Shift-click another to select every command block between them (highlighted blue).
  - Right-click to apply a context-menu action to all of them.
  - When branches are involved, a range can only cover rows inside the same branch.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-EVT-048 — Event command picker: tabs and categories
- **What:** The window used to choose a command. Commands are spread over three tabs and grouped by category.
- **Behavior:**
  - Tab 1: Message, Game Progression, Flow Control, Party, Actor.
  - Tab 2: Movement, Character, Picture, Timing, Screen, Audio & Video.
  - Tab 3: Scene Control, System Settings, Map, Battle, Advanced.
  - Any of these commands can be used in an event's [Contents].
  - This document covers Message through Picture. Timing, Screen, Audio & Video and all of Tab 3 are listed on the overview but belong to other slices.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10.html

---

### AREA CMD — Event commands (help order)

### RM-CMD-001 — Messages: Show Text
- **What:** Opens the message window and shows a block of text, optionally with a face graphic and a speaker name.
- **Behavior:**
  - Face: face graphic drawn on the left side of the message window. Picked by double-clicking the face box ([Select an Image]).
  - Text: up to 4 lines fit in the window at once. Enter starts a new line.
  - Text right-click menu: [Plugin Help] (shows plugin help), [Insert Color Number] (pick a colour and its number is inserted), [Insert Icon Number] (pick an icon and its number is inserted).
  - Name: a speaker name shown above the message (name box).
  - Background: the style of the message window. Default "Window" **(screenshot)**. The other choices are not listed in the text.
  - Window Position: Bottom, Middle or Top. Default Bottom **(screenshot)**.
  - Preview: shows how the message window will look.
  - Batch Entry: when checked, more than 4 lines can be typed. On OK the text is split into several Show Text commands of 4 lines each.
  - Ctrl+Enter in the text box acts like OK.
  - A vertical guide line in the text box marks the window's maximum line width. Use Preview to confirm the layout.
  - Supports control characters (see the MSG area). They must be typed as half-width characters.
  - Other places that accept the codes \V, \N, \P, \G, \C, \I, \{, \}, \\, \PX, \PY and \FS:
    - Show Choices text.
    - Actor [Profile].
    - Skill [Message].
    - Item, Weapon and Armor [Description].
    - State [Messages].
    - Terms → [Save Message] and [Load Message].
    - Terms → battle messages from [Party Name] onward. For text shown in that window the help also lists \$ \. \| \! \> \< \^.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-CMD-002 — Messages: Show Choices
- **What:** Shows a choice window with up to six options and branches the event by the player's pick.
- **Behavior:**
  - Choices #1–#6: text for each option. Blank options are not shown. Control characters are allowed.
  - Background: style of the choice window. Default "Window" **(screenshot)**.
  - Window Position: where the choice window appears. Shown set to "Right" **(screenshot)**. The full list of positions is not in the text.
  - Default: which option the cursor starts on. Shown as "Choice #1" **(screenshot)**.
  - Cancel: what pressing cancel does. Shown set to "Choice #2", i.e. cancel can be mapped to an option **(screenshot)**. The full list of cancel options is not in the text.
  - On OK, the editor generates a branch in the contents for every choice and also for Cancel. The commands for each outcome go inside those branches.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-CMD-003 — Messages: Input Number
- **What:** Shows a number-entry window and stores the entered number in a variable.
- **Behavior:**
  - Variable: where the number is stored.
  - Digits: number of digits accepted, from 1 to 8. Default 1 **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-CMD-004 — Messages: Select Item
- **What:** Shows an item-picker window and stores the ID of the chosen item in a variable.
- **Behavior:**
  - Variable: receives the item ID.
  - Item Type: which category of items is listed. Default "Key Item" **(screenshot)**. The other types are not listed in the text.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-CMD-005 — Messages: Show Scrolling Text
- **What:** Shows text that scrolls up the screen from the bottom (credits-style).
- **Behavior:**
  - Text: the content.
  - Text right-click menu: [Plugin Help], [Icon Set Viewer].
  - Speed: 1–8. Higher is faster. Default 2 **(screenshot)**.
  - Pressing the confirm button fast-forwards the scroll, unless No Fast Forward is checked.
  - A Preview button is present **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-CMD-006 — Game Progression: Control Switches
- **What:** Sets one switch or a range of switches ON or OFF.
- **Behavior:**
  - Switch: [Single] (one switch) or [Range] (a numbered from–to span). Default Single **(screenshot)**.
  - Operation: ON or OFF. Default ON **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html

### RM-CMD-007 — Game Progression: Control Variables
- **What:** Changes one variable or a range of variables using an operation and an operand.
- **Behavior:**
  - Variable: [Single] or [Range] (from–to).
  - Operations:
    - Set: store the operand.
    - Add: old value + operand.
    - Sub: old value − operand.
    - Mul: old value × operand.
    - Div: old value ÷ operand.
    - Mod: remainder of old value ÷ operand.
    - Default Set **(screenshot)**.
  - Operand types:
    - Constant: a typed number. Default 0 **(screenshot)**.
    - Variable: the value of another variable.
    - Random: a random number between a min and max field **(screenshot)**. The generated range can be anywhere from −99,999,999 to 99,999,999.
    - Game Data: a game-state value picked in a [...] dialog (sub-types below).
    - Script: the result of evaluating a JavaScript expression.
  - Game Data sub-types:
    - Items: how many of an item the party holds.
    - Weapons: how many of a weapon the party holds.
    - Armor: how many of an armor the party holds.
    - Actors: a chosen parameter (HP, MP, etc.) of a chosen actor.
    - Enemies: a chosen parameter (HP, MP, etc.) of a chosen enemy. Only meaningful during battle.
    - Character: a value from the player or an event:
      - Map X / Map Y: tile coordinates.
      - Direction: down = 2, left = 4, right = 6, up = 8.
      - Screen X / Screen Y: on-screen pixel position.
    - Party: the actor ID of the party member in a chosen slot.
    - Just Prior (last action):
      - ID of the skill last used.
      - ID of the item last used.
      - ID of the actor who last acted.
      - Index of the enemy who last acted.
      - ID of the actor last targeted.
      - Index of the enemy last targeted.
    - Other: "other" data. The help does not list the entries.
  - Battle events: not stated, except that the Enemies operand only works in battle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html

### RM-CMD-008 — Game Progression: Control Self Switch
- **What:** Sets a self switch ON or OFF.
- **Behavior:**
  - Self Switch: A, B, C or D. Default A **(screenshot)**.
  - Operation: ON or OFF. Default ON **(screenshot)**.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html

### RM-CMD-009 — Game Progression: Control Timer
- **What:** Starts or stops a countdown timer.
- **Behavior:**
  - Operation: Start or Stop. Default Start **(screenshot)**.
  - Duration (only for Start): from 0 min 0 sec up to 99 min 59 sec, entered as separate minute and second fields **(screenshot)**.
  - While running, the remaining time is shown in the top-right corner of the screen.
  - The timer pauses (stops counting) while the menu is open.
  - To act on the remaining time, use Conditional Branch (Timer condition).
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html

### RM-CMD-010 — Flow Control: Conditional Branch
- **What:** Runs a block of commands only when a condition holds, optionally with an "else" block.
- **Behavior:**
  - Dialog has four tabs (1–4) of condition types **(screenshot)**. Tab 1 shows Switch, Variable, Self Switch and Timer **(screenshot)**.
  - Create Else Branch: adds a block that runs when the condition is false.
  - Conditions:
    - Switch: a chosen switch is ON or OFF.
    - Variable: a variable compared to a constant or to another variable, using a chosen comparison operator. The operator list is not given in the text.
    - Self Switch: a chosen self switch is ON or OFF.
    - Timer: remaining time compared to a given time (help text: greater than / less than).
    - Actors: about a chosen actor. Check one of: in party, name, class, skill, weapon, armor, state, each with its own target.
    - Enemies: about a chosen enemy in battle (has appeared / has a state). Only works in battle events.
    - Character: the player's or an event's facing direction. Only works in map events.
    - Vehicle: the player is riding a chosen vehicle (boat / ship / airship).
    - Gold: party gold compared to an amount (greater than / less than).
    - Items: the party holds a chosen item.
    - Weapons: the party holds a chosen weapon. [Include Equipped] also counts weapons equipped on actors.
    - Armor: the party holds a chosen armor. [Include Equipped] also counts equipped armor.
    - Button: a chosen button's state:
      - [Pressed]: true for as long as the button is held.
      - [Triggered]: true only on the frame it is first pressed.
      - [Repeated]: while held, true on the first press, again 24 frames later, then every 6 frames.
    - Script: the result of evaluating a script (for advanced users). Right-click → [Plugin Help].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-011 — Flow Control: Loop
- **What:** Inserts a [Loop] … [Repeat Above] pair. The commands between them repeat.
- **Behavior:**
  - Repeats forever until something exits it, e.g. [Break Loop].
  - Loops can be nested inside other loops.
  - No settings dialog is described.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-012 — Flow Control: Break Loop
- **What:** Leaves the current loop.
- **Behavior:**
  - Execution continues with the command after the matching [Repeat Above].
  - No parameters.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-013 — Flow Control: Exit Event Processing
- **What:** Ends the current event's contents immediately.
- **Behavior:**
  - No later commands in the list run.
  - No parameters.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-014 — Flow Control: Common Event
- **What:** Calls a common event as a subroutine.
- **Behavior:**
  - Common Event: which one to run (dropdown) **(screenshot)**.
  - The caller resumes with its next command only after the called common event has finished.
  - Listed as "Common Events" on the overview page.
  - Battle events: not stated (the Events page says common events can be used from both map and battle events).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_09.html

### RM-CMD-015 — Flow Control: Label
- **What:** Marks a named point that [Jump to Label] can jump to.
- **Behavior:**
  - Label Name: the label's name.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-016 — Flow Control: Jump to Label
- **What:** Moves execution to a named label.
- **Behavior:**
  - Label Name: the target label.
  - Execution continues with the command right after that label.
  - Only labels on the same event page can be targeted.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-017 — Flow Control: Comment
- **What:** A note shown in the contents list that does nothing in the game.
- **Behavior:**
  - Comment: free text.
  - Right-click → [Plugin Help] shows plugin help.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

### RM-CMD-018 — Party: Change Gold
- **What:** Adds or removes party gold.
- **Behavior:**
  - Operation: Increase or Decrease. Default Increase **(screenshot)**.
  - Operand: [Constant] (typed amount, default 1 **(screenshot)**) or [Variable] (amount read from a variable).
  - Gold limits are not stated.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-CMD-019 — Party: Change Items
- **What:** Adds or removes a quantity of one item.
- **Behavior:**
  - Items: which item.
  - Operation: Increase or Decrease.
  - Operand: Constant or Variable.
  - The resulting count is clamped to 0–99 (99 is the most the party can hold, 0 means none).
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-CMD-020 — Party: Change Weapons
- **What:** Adds or removes a quantity of one weapon.
- **Behavior:**
  - Weapons: which weapon.
  - Operation: Increase or Decrease.
  - Operand: Constant or Variable.
  - The result is clamped to 0–99.
  - Include Equipped: when checked, a decrease may also take weapons that actors have equipped. In the screenshot the checkbox (labeled "Include Equipment") is grayed out while Increase is selected **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-CMD-021 — Party: Change Armor
- **What:** Adds or removes a quantity of one armor.
- **Behavior:**
  - Armor: which armor.
  - Operation: Increase or Decrease.
  - Operand: Constant or Variable.
  - The result is clamped to 0–99.
  - Include Equipped: when checked, a decrease may also take armor that actors have equipped.
  - The help's Function line for this command says "weapons", which looks like a copy error. Its settings are all about armor.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-CMD-022 — Party: Change Party Member
- **What:** Adds an actor to the party or removes one.
- **Behavior:**
  - Actors: which actor.
  - Operation: Add or Remove. Default Add **(screenshot)**.
  - Initialize: when adding, resets the actor to the state defined in the database.
  - The party may be emptied to 0 members. In that case no player character is drawn on the map.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-CMD-023 — Actor: Change HP
- **What:** Raises or lowers HP of an actor or the whole party.
- **Behavior:**
  - Actor:
    - [Fixed]: pick an actor, or [Entire Party] (the default **(screenshot)**).
    - [Variable]: the actor ID is read from a variable.
  - Operation: Increase or Decrease.
  - Operand: Constant (1–9999) or Variable.
  - Allow Death: when checked, an actor whose HP reaches 0 or less is knocked out. When unchecked, HP stops at 1 instead. The checkbox is grayed out while Increase is selected **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-024 — Actor: Change MP
- **What:** Raises or lowers MP of an actor or the whole party.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable (ID from a variable).
  - Operation: Increase or Decrease.
  - Operand: Constant (1–9999) or Variable.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-025 — Actor: Change TP
- **What:** Raises or lowers TP of an actor or the whole party.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Operation: Increase or Decrease.
  - Operand: Constant (1–100) or Variable.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-026 — Actor: Change State
- **What:** Adds a state to an actor (or the whole party) or removes one.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Operation: Add or Remove. Default Add **(screenshot)**.
  - State: which state. The first entry in the screenshot list is "0001 Dead" **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-027 — Actor: Recover All
- **What:** Fully heals an actor or the whole party.
- **Behavior:**
  - Restores HP and MP to full and removes every state.
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-028 — Actor: Change EXP
- **What:** Raises or lowers experience points.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Operation: Increase or Decrease.
  - Operand: Constant (1–9,999,999) or Variable.
  - Show Level Up: when checked, a level-up message appears on the play screen if the gain causes a level-up.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-029 — Actor: Change Level
- **What:** Raises or lowers level.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Operation: Increase or Decrease.
  - Operand: Constant (1–98) or Variable.
  - Show Level Up: when checked, a level-up message appears on the play screen when the level goes up.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-030 — Actor: Change Parameter
- **What:** Raises or lowers one base parameter.
- **Behavior:**
  - Actor: Fixed (an actor or Entire Party) or Variable.
  - Parameter: which stat. Default "Max HP" **(screenshot)**. The full list is not in the text.
  - Operation: Increase or Decrease.
  - Operand: Constant (1–9999) or Variable.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-031 — Actor: Change Skill
- **What:** Teaches an actor a skill or removes one.
- **Behavior:**
  - Actor: Fixed or Variable. The text only mentions a specific actor, but the screenshot's Fixed dropdown shows [Entire Party] **(screenshot)**.
  - Operation: [Learn] (skill becomes usable) or [Forget] (skill becomes unusable).
  - Skill: which skill.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-032 — Actor: Change Equipment
- **What:** Changes what an actor has equipped in one slot.
- **Behavior:**
  - Actor: a specific actor (dropdown).
  - Equipment Type: which slot. Default "Weapon" **(screenshot)**.
  - Equipment Item: what to equip. The list includes "None" **(screenshot)**.
  - The party must own the item. If it does not, nothing changes. To force an item onto an actor, first give it to the party with Change Weapons / Change Armor.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-033 — Actor: Change Name
- **What:** Renames an actor.
- **Behavior:**
  - Actor: which actor.
  - Name: the new name.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-034 — Actor: Change Class
- **What:** Moves an actor to another class.
- **Behavior:**
  - Actor: which actor.
  - Class: the new class.
  - Save Level: when checked, the actor keeps their current level after the change.
  - Any equipment the new class cannot use is removed automatically.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-035 — Actor: Change Nickname
- **What:** Sets an actor's nickname.
- **Behavior:**
  - Actor: which actor.
  - Nickname: the new nickname.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-036 — Actor: Change Profile
- **What:** Sets an actor's profile (bio) text.
- **Behavior:**
  - Actor: which actor.
  - Profile: the new text (a multi-line box **(screenshot)**).
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-CMD-037 — Movement: Transfer Player
- **What:** Moves the party to another location, which can be on another map.
- **Behavior:**
  - Location:
    - [Direct Designation]: a [...] map picker where you click the destination.
    - [Designation with Variables]: variables hold the map ID, X and Y.
  - Direction: the player's facing after arrival. Default "Retain" **(screenshot)**. The other values are not listed in the text.
  - Fade: the transition style. [Black] fades through black, [White] through white. Default Black **(screenshot)**. Whether there is a no-fade option is not stated.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-038 — Movement: Set Vehicle Location
- **What:** Moves a vehicle to a new spot.
- **Behavior:**
  - Vehicle: boat, ship or airship ("plane" in the help text). Default "Boat" **(screenshot)**.
  - Location: [Direct Designation] (map picker) or [Designation with Variables] (variables for map ID, X, Y).
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-039 — Movement: Set Event Location
- **What:** Moves an event to another tile on the same map.
- **Behavior:**
  - Event: the target event, or [This Event] for the event running the command.
  - Destinations are limited to the current map.
  - Location:
    - [Direct Designation]: map picker.
    - [Designation with Variables]: X and Y variables.
    - The screenshot also shows a third option, "Exchange with another event", with an event dropdown. The text does not describe it **(screenshot)**.
  - Direction: facing after the move. Default "Retain" **(screenshot)**. The help text says "player" here, apparently meaning the moved event.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-040 — Movement: Scroll Map
- **What:** Pans the map view without moving the player.
- **Behavior:**
  - Direction: which way to pan. Default "Down" **(screenshot)**.
  - Distance: how many tiles. Default 1 **(screenshot)**.
  - Speed: six levels. Default "4: Normal" **(screenshot)**.
  - Wait for Completion: when checked, the event pauses until the pan finishes.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-041 — Movement: Set Movement Route
- **What:** Forces the player or a map event to follow a scripted route (the editor is described in the MRT area).
- **Behavior:**
  - The character starts the route immediately.
  - Issuing a new route to a character that is already on one throws the old route away; only the new one applies. To chain routes, use [Wait for Completion] on the earlier one.
  - With [Wait for Completion] on, the player cannot act until the route ends (parallel processes still run). If the character gets blocked and cannot continue, the event stalls at that point. Options such as [Skip If Cannot Move] avoid this.
  - Moving the player this way does not add to the step count.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-042 — Movement: Get On/Off Vehicle
- **What:** Toggles boarding: leaves the vehicle if on one, boards a vehicle if not.
- **Behavior:**
  - No parameters.
  - Works the same as the player pressing the action key to board or exit.
  - Does nothing if there is no vehicle to board or no ground to step off onto.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06.html

### RM-CMD-043 — Character: Change Transparency
- **What:** Hides or shows the party on the map.
- **Behavior:**
  - Transparency: [ON] makes the party invisible; [OFF] makes it visible again.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-044 — Character: Change Player Followers
- **What:** Turns on or off the display of party members 2+ walking behind the leader.
- **Behavior:**
  - Player Followers: [ON] shows followers; [OFF] hides them.
  - Even with five or more members, only the first four actors are shown on the map.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-045 — Character: Gather Followers
- **What:** Moves every follower onto the leader's tile.
- **Behavior:**
  - No parameters.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-046 — Character: Show Animation
- **What:** Plays an animation on the map at a character's position.
- **Behavior:**
  - Character: the player or an event the animation is anchored to. Default "Player" **(screenshot)**.
  - Animation: which animation.
  - Wait for Completion: when checked, the event pauses until the animation ends.
  - Cannot be used in battle events (stated). Use [Show Battle Animation] in battle instead.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-047 — Character: Show Balloon Icon
- **What:** Shows an emotion balloon above the player/party member or an event.
- **Behavior:**
  - Character: who the balloon appears over. Default "Player" **(screenshot)**.
  - Balloon Icon: which balloon. Ten built-in kinds, e.g. "!" for surprise and a light bulb for an idea. Default "Exclamation" **(screenshot)**. Users can define their own balloons.
  - Wait for Completion: when checked, the event pauses until the balloon disappears.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-048 — Character: Erase Event
- **What:** Temporarily removes the event that is running the command.
- **Behavior:**
  - No parameters.
  - The event stays gone until the party moves to another map.
  - Cannot be used in battle events (stated).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_07.html

### RM-CMD-049 — Picture: Show Picture
- **What:** Shows a still image on screen in a numbered picture slot.
- **Behavior:**
  - Number: slot 1–100. Default 1 **(screenshot)**.
  - Image: the picture file.
  - Position:
    - Origin: [Upper Left] or [Center]. Default Upper Left **(screenshot)**. (The help's wording mentions both the image's origin point and "of the screen".)
    - [Direct Designation]: X and Y from −9999 to 9999. Default 0, 0 **(screenshot)**.
    - [Designation with Variables]: X and Y read from variables.
  - Scale: Width % and Height %, each from −2000 to 2000. 100 = original size, the default **(screenshot)**. A negative value flips the image.
  - Blend:
    - Opacity: 0–255. Lower is more transparent; 0 is invisible. Default 255 **(screenshot)**.
    - Blend Mode: Normal, Additive (lighter/whiter), Multiply (darker), Screen (brighter). Default Normal **(screenshot)**.
  - Up to 100 pictures can be on screen at once. Higher slot numbers draw in front.
  - Showing a picture in a slot that is already in use replaces the earlier one.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html

### RM-CMD-050 — Picture: Move Picture
- **What:** Moves a displayed picture over time to new position, scale, opacity and blend values.
- **Behavior:**
  - Number: slot 1–100.
  - Easing: four choices:
    - Constant speed (the text calls it "Fixed Speed"; default **(screenshot)**).
    - Start slow.
    - Stop slow.
    - Start slow and stop slow.
  - Position: origin (Upper Left/Center) plus final X/Y, either typed (−9999 to 9999) or from variables.
  - Scale: final Width % and Height %, −2000 to 2000. Negative values flip.
  - Blend: final Opacity (0–255) and Blend Mode (Normal / Additive / Multiply / Screen).
  - Duration: 1–999 frames (60 frames = 1 second). Default 60 **(screenshot)**.
  - Wait for Completion: when checked, the event waits for the move to finish. Checked by default **(screenshot)**.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html

### RM-CMD-051 — Picture: Rotate Picture
- **What:** Spins a displayed picture continuously.
- **Behavior:**
  - Number: slot 1–100.
  - Speed: −90 to 90. Positive turns clockwise, negative turns counter-clockwise, 0 stops. Default 0 **(screenshot)**.
  - The picture turns around the origin set by its most recent Show Picture or Move Picture.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html

### RM-CMD-052 — Picture: Tint Picture
- **What:** Changes a displayed picture's colour tone over time.
- **Behavior:**
  - Number (also called "ID" in the help): slot 1–100.
  - Color Tone:
    - Red, Green and Blue offsets, each from −255 to 255.
    - Gray (desaturation strength) from 0 to 255.
    - Higher values give a stronger tint.
    - A preview box sits on the right.
    - Preset buttons: Normal, Dark, Sepia, Sunset, Night **(screenshot)**.
  - Duration: 1–999 frames. Default 60 **(screenshot)**.
  - Wait for Completion: checked by default **(screenshot)**.
  - The tint stays until another Tint Picture changes it.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html

### RM-CMD-053 — Picture: Erase Picture
- **What:** Removes a displayed picture.
- **Behavior:**
  - Number/ID: slot 1–100.
  - Battle events: not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_08.html

---

### AREA MSG — Message control characters

All codes below come from the Show Text control-character table. They must be typed as half-width characters. Show Text and Show Choices accept them, and so do the database fields listed in RM-CMD-001. That list covers \V \N \P \G \C \I \{ \} \\ \PX \PY \FS everywhere, plus \$ \. \| \! \> \< \^ in battle-message terms.

### RM-MSG-001 — `\V[n]` (variable value)
- **What:** Inserts the current value of variable n.
- **Behavior:** Replaced by the number stored in variable n when the text is shown.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-002 — `\N[n]` (actor name)
- **What:** Inserts the name of actor n (database actor ID).
- **Behavior:** Replaced by that actor's current name.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-003 — `\P[n]` (party member name)
- **What:** Inserts the name of the party member in slot n of the current party order.
- **Behavior:** Uses party order, not database ID.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-004 — `\G` (currency unit)
- **What:** Inserts the game's currency unit name.
- **Behavior:** No argument.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-005 — `\C[n]` (text colour)
- **What:** Draws the following text in colour n.
- **Behavior:**
  - The colour comes from the palette in the system image "Window.png".
  - Applies to all text after it until changed again.
  - The editor's right-click [Insert Color Number] helps choose n.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-006 — `\I[n]` (icon)
- **What:** Draws icon n inline in the text.
- **Behavior:** The editor's right-click [Insert Icon Number] helps choose n.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-007 — `\{` (bigger text)
- **What:** Makes the following text one step larger.
- **Behavior:** No argument.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-008 — `\}` (smaller text)
- **What:** Makes the following text one step smaller.
- **Behavior:** No argument.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-009 — `\\` (literal backslash)
- **What:** Prints a backslash character instead of starting a code.
- **Behavior:** Escape for the code prefix.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-010 — `\$` (gold window)
- **What:** Opens the gold (money) window while the message is shown.
- **Behavior:** No argument.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-011 — `\.` (short wait)
- **What:** Pauses the text for a quarter of a second.
- **Behavior:** 1/4 second wait at that point.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-012 — `\|` (long wait)
- **What:** Pauses the text for one second.
- **Behavior:** 1 second wait at that point.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-013 — `\!` (wait for input)
- **What:** Pauses the text until the player presses a button.
- **Behavior:** No argument.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-014 — `\>` (instant display on)
- **What:** Shows the rest of the current line all at once instead of letter by letter.
- **Behavior:** Stays in effect until cancelled with `\<`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-015 — `\<` (instant display off)
- **What:** Ends the instant-display effect started by `\>`.
- **Behavior:** Text goes back to normal letter-by-letter output.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-016 — `\^` (no input wait)
- **What:** The message does not wait for the player to press a button once it has finished showing.
- **Behavior:** The help's wording is terse ("after displaying the next"). In effect, the message closes or proceeds without waiting for confirmation.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-017 — `\PX[n]` (text X position)
- **What:** Moves the text drawing position to horizontal coordinate n.
- **Behavior:** Measured from the window's top-left corner as origin.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-018 — `\PY[n]` (text Y position)
- **What:** Moves the text drawing position to vertical coordinate n.
- **Behavior:** Measured from the window's top-left corner as origin.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSG-019 — `\FS[n]` (font size)
- **What:** Sets the text size to n.
- **Behavior:** Absolute size, unlike the relative steps of `\{` / `\}`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

---

### AREA MRT — Movement route editor, commands and options

General rules for every MRT item (from 01_10_06_01):
- The same route editor serves both a page's [Custom] autonomous movement and the [Set Movement Route] command.
- One step = one tile.
- Clicking a command button inserts it at the highlighted row of the command list.
- Commands run in list order.
- Command names below follow the help text. Where the editor button label differs, the label is noted **(screenshot)**.

### RM-MRT-001 — Route editor: target selector
- **What:** A dropdown at the top-left that picks whose route this is: the player or an event.
- **Behavior:**
  - Only shown when editing via the [Set Movement Route] command (not for a page's Custom movement).
  - Default "Player" **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-002 — Route editor: Preview
- **What:** Plays the route on the map as a preview.
- **Behavior:** Opened with the [Preview...] button.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-003 — Route editor: command list
- **What:** The ordered list of route commands that have been added.
- **Behavior:**
  - New commands go in at the highlighted row.
  - Right-clicking a row opens a menu with actions such as re-editing or copying.
  - An empty list shows one "◇" row **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-004 — Move Down
- **What:** One step down.
- **Behavior:** Moves one tile in the named direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-005 — Move Left
- **What:** One step left.
- **Behavior:** Moves one tile in the named direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-006 — Move Right
- **What:** One step right.
- **Behavior:** Moves one tile in the named direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-007 — Move Up
- **What:** One step up.
- **Behavior:** Moves one tile in the named direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-008 — Move Lower Left
- **What:** One diagonal step down-left.
- **Behavior:** Moves one tile in the named diagonal direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-009 — Move Lower Right
- **What:** One diagonal step down-right.
- **Behavior:** Moves one tile in the named diagonal direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-010 — Move Upper Left
- **What:** One diagonal step up-left.
- **Behavior:** Moves one tile in the named diagonal direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-011 — Move Upper Right
- **What:** One diagonal step up-right.
- **Behavior:** Moves one tile in the named diagonal direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-012 — Move at Random
- **What:** One step in a random direction.
- **Behavior:** The direction is randomly one of the four cardinal directions (up, down, left, right).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-013 — Move Toward Player
- **What:** One step toward the player.
- **Behavior:** Single step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-014 — Move Away From Player
- **What:** One step away from the player.
- **Behavior:** Single step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-015 — 1 Step Forward
- **What:** One step in the direction the character currently faces.
- **Behavior:** Single step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-016 — 1 Step Backward
- **What:** One step opposite to the direction the character faces.
- **Behavior:** Single step.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-017 — Jump
- **What:** Jumps by a relative offset.
- **Behavior:**
  - X and Y offsets, each from −100 to 100. Positive X is right; positive Y is down.
  - The jump ignores obstacles along the way.
  - If [Show Player Followers] is ON, followers (members 2+) do not jump with the leader; they stay where they were.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-018 — Wait
- **What:** Pauses the route for a number of frames.
- **Behavior:** 1–999 frames; 60 frames = 1 second.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-019 — Face Down (button: "Turn Down")
- **What:** Turns to face down without moving.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-020 — Face Left (button: "Turn Left")
- **What:** Turns to face left without moving.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-021 — Face Right (button: "Turn Right")
- **What:** Turns to face right without moving.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-022 — Face Up (button: "Turn Up")
- **What:** Turns to face up without moving.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-023 — Turn 90° Right
- **What:** Turns a quarter turn clockwise (to the right).
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-024 — Turn 90° Left
- **What:** Turns a quarter turn counter-clockwise (to the left).
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-025 — Turn 180°
- **What:** Turns to face the opposite way.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-026 — Turn 90° Right or Left
- **What:** Turns 90° randomly to one side or the other.
- **Behavior:** Random quarter turn.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-027 — Turn at Random
- **What:** Faces a random one of up/down/left/right.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-028 — Turn toward Player
- **What:** Faces in the player's direction.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-029 — Turn away from Player
- **What:** Faces away from the player.
- **Behavior:** Changes facing only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-030 — Switch ON
- **What:** Turns a chosen (global) switch ON from inside the route.
- **Behavior:** Switch selected via a dialog (button "Switch ON...") **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-031 — Switch OFF
- **What:** Turns a chosen (global) switch OFF from inside the route.
- **Behavior:** Switch selected via a dialog (button "Switch OFF...") **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-032 — Change Speed
- **What:** Changes the target's autonomous-movement [Speed].
- **Behavior:**
  - The new speed stays after the route has finished.
  - The level is picked in a dialog. The level list is not given in the text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-033 — Change Frequency
- **What:** Changes the target's autonomous-movement [Frequency].
- **Behavior:** The new frequency stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-034 — Walking Animation ON
- **What:** Turns the target's [Walking] option on.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-035 — Walking Animation OFF
- **What:** Turns the target's [Walking] option off.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-036 — Stepping Animation ON
- **What:** Turns the target's [Stepping] option on.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-037 — Stepping Animation OFF
- **What:** Turns the target's [Stepping] option off.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-038 — Direction Fix ON
- **What:** Turns the target's [Direction Fix] option on.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-039 — Direction Fix OFF
- **What:** Turns the target's [Direction Fix] option off.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-040 — Through ON
- **What:** Turns the target's [Through] option on (pass through obstacles).
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-041 — Through OFF
- **What:** Turns the target's [Through] option off.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-042 — Transparent ON
- **What:** Hides the character's graphic.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-043 — Transparent OFF
- **What:** Shows the character's graphic again.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-044 — Change Image
- **What:** Swaps the character's graphic for a chosen image.
- **Behavior:** Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-045 — Change Opacity
- **What:** Sets the character graphic's opacity.
- **Behavior:**
  - 0–255; 0 is fully invisible.
  - Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-046 — Change Blend Mode
- **What:** Sets how the character's colors blend with the map underneath.
- **Behavior:**
  - Modes: Normal; Additive (lighter/whiter); Multiply (darker); Screen (brighter).
  - Stays after the route has finished.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-047 — Play SE
- **What:** Plays a chosen sound effect at that point in the route.
- **Behavior:** SE picked in a dialog.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-048 — Script
- **What:** Runs an entered script as a route step.
- **Behavior:** The script text is entered in a dialog.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-049 — Option: Repeat Movements
- **What:** Loops the whole command list.
- **Behavior:**
  - When checked, the list runs again from the start after it finishes, repeatedly.
  - Unchecked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-050 — Option: Skip If Cannot Move
- **What:** Skips blocked steps instead of waiting at them.
- **Behavior:**
  - When checked, any move that cannot be made (e.g. blocked by an obstacle) is skipped.
  - Unchecked by default **(screenshot)**.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

### RM-MRT-051 — Option: Wait for Completion
- **What:** Holds other processing until the whole route has finished.
- **Behavior:**
  - Only available for the [Set Movement Route] command (not for a page's Custom movement).
  - Checked by default **(screenshot)**.
  - With it on, a route that gets blocked stalls the event (see RM-CMD-041).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_06_01.html

---

## 3. Not verified

Not stated on the pages fetched, or unclear or contradictory. Do not implement any of these from these notes without another official source.

1. **Event searcher / event search tool:** not mentioned on 01_09_02 (or elsewhere in this slice).
2. **Page condition Variable comparison:** the 01_09_03 text says "greater than", but the screenshot shows "≥". Which one is exact is not verified.
3. **Page condition: two Switch slots:** seen only in the screenshot. The text describes a single "Switch" condition.
4. **Speed and Frequency level lists** (autonomous movement and route Change Speed/Frequency): the full lists and names are not given. Only "3: x2 Slower" (speed), "3: Normal" (frequency) and the Scroll Map speed "4: Normal" were seen in screenshots.
5. **Autorun trigger:** whether it blocks player control, and whether it repeats, is not described on 01_09_03.
6. **When page conditions are re-evaluated** (refresh timing): not stated.
7. **Quick Events:** the exact commands each template generates (e.g. which self switch Treasure uses, the Inn's message and fade flow, the Door's open animation) are not stated. The help also uses two names for one template: "Transfer" in the intro and "Movement" in its heading.
8. **Show Text Background options** other than "Window" (e.g. dimmed or transparent): not listed.
9. **Name box details** (position, whether control characters work in [Name]): not stated.
10. **Show Choices:** the Window Position values (only "Right" seen), the Background values (only "Window" seen), and the Cancel options other than choosing a choice (e.g. disallow cancel, or a separate cancel branch). The remark says a Cancel branch is created, but the option names are not listed.
11. **Select Item:** the Item Type list (only "Key Item" seen).
12. **Control Variables:** how Div/Mod round and handle division by zero, variable min/max limits, the full Actors and Enemies parameter lists, and the entries under Game Data "Other".
13. **Conditional Branch:**
    - the Variable comparison operators;
    - the exact Timer and Gold comparisons (help says greater than / less than, which may in fact be inclusive);
    - the list of Button names;
    - the contents of tabs 2–4 (only tab 1 seen in the screenshot);
    - the exact Actor sub-conditions, beyond the list given (in party, name, class, skill, weapon, armor, state).
14. **Control Self Switch:** that it targets the event running the command is implied, not stated.
15. **Change Gold:** upper or lower limits for gold are not stated.
16. **Change Parameter:** the list of parameters (only "Max HP" seen).
17. **Change Equipment:** whether picking "None" unequips (the option is seen in the screenshot only), and the list of equipment types (only "Weapon" seen).
18. **Change Skill:** [Entire Party] is seen in the screenshot only; the text describes a specific actor.
19. **Transfer Player:** the Direction values other than "Retain", and whether there is a "None" fade option.
20. **Set Event Location:** what "Exchange with another event" does (screenshot only).
21. **Show Balloon Icon:** the names of all ten built-in balloons (only "Exclamation" and the light-bulb example are given). How user-defined balloons are added is not stated.
22. **Picture Origin:** the help says the origin is "Upper Left or Center of the screen" while also calling it the image's origin point. The exact meaning is not verified.
23. **Battle-event usability:** not stated for Show Text, Show Choices, Input Number, Select Item, Show Scrolling Text, Control Switches, Control Variables, Control Timer, every Flow Control command, every Party and Actor command, Transfer Player, Set Vehicle Location, Change Transparency, Change Player Followers, Gather Followers, Show Balloon Icon, and every Picture command.
24. **`\^` exact semantics:** the help's sentence is terse. "Proceeds without waiting for input" is our reading.
25. **Font size step amount** for `\{` / `\}`, and the units of `\FS[n]`, `\PX[n]` and `\PY[n]`: not stated.
