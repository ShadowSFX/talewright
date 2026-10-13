# RPG Maker MZ Feature Inventory: Database (part B)

Enemies, Troops (with battle events), States, Animations (with Effekseer and MV-format animations), Common Events.

Clean-room note: every item below is written in our own words from official sources fetched on 2026-10-13. Field names are kept as the editor shows them. No prose is copied and there is no code. When a page does not state something, the item or the final list says "not verified" instead of filling the gap from memory.

Method note: each primary help page was fetched with WebFetch, once for a full pass and once more with narrow questions. Because WebFetch hands back processed text, the raw HTML of the same official URLs was also downloaded and the tags stripped to check exact field names and numbers. Each page also has an editor screenshot (`assets/img/<page>_imgNN.png` on the same help site). These were downloaded and viewed. Anything learned only from a screenshot is marked "(screenshot)" and cites the image URL, because the page text does not say it.

## 1. Pages covered

### Primary pages (this slice)

| Page | URL | Load status |
|---|---|---|
| Enemy Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html | Loaded (WebFetch 2x plus raw HTML) |
| Troop Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html | Loaded (WebFetch 2x plus raw HTML) |
| State Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html | Loaded (WebFetch 2x plus raw HTML) |
| Animation Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html | Loaded (WebFetch 2x plus raw HTML) |
| Effekseer | https://rpgmakerofficial.com/product/MZ_help-en/01_08_09_01.html | Loaded (WebFetch plus raw HTML) |
| Common Event Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html | Loaded (WebFetch 2x plus raw HTML) |

### Screenshots viewed (official help assets)

| Image | URL | Shows |
|---|---|---|
| Enemy editor | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img01.png | Enemy tab layout |
| Enemy Action dialog | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png | Action pattern dialog |
| Troop editor | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png | Troop tab layout |
| Troop Conditions dialog | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png | Battle event conditions dialog |
| State editor | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png | State tab layout |
| Animation editor | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png | Animation tab layout |
| Database window, Animations tab | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_01_01_04_img01.png | Same animation layout inside the full Database window |
| Common Event editor | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_11_img01.png | Common Event tab layout |
| Side-view motion sheet | https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_11_02_img02.png | Named side-view motions |

### Linked or related pages followed for detail

| Page | URL | Load status | Use |
|---|---|---|---|
| Database (overview) | https://rpgmakerofficial.com/product/MZ_help-en/01_08.html | Loaded | Entry limits; which data types have a Notes field |
| How to Set Traits (linked from Enemies, States) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html | Loaded | Cross-reference only. The full trait inventory is in another slice (05_database_c.md). |
| Event page editing, map events (linked as `01_09_03.html#p03` from Troops, Common Events) | https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html | Loaded | Event page buttons and how event commands are edited, which troops and common events reuse |
| New Functions: Animations | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_04.html | Loaded | Where animations are used |
| New Functions: MV Data Conversion | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html | Loaded | Help's statement on MV animations |
| New Functions: Improvements to Artistic Expressions | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html | Loaded | Troop enemy ID display; multiple common-event effects |
| Asset Standards | https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html | Loaded | Enemy image folders; States.png |
| Side-View Character Standards | https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html | Loaded | Side-view motion sheet (relevant to the state [SV] Motion field) |
| Official blog: "Making MV Style Animations" (rpgmakerweb.com, by hiddenone; the page shows the dates 2022-01-11 and 2026-06-10) | https://www.rpgmakerweb.com/blog/making-mv-style-animations | Loaded (WebFetch plus raw HTML) | MV-format animation editor in MZ |
| Official forum: "RPG Maker MZ 1.4.0 Update" (staff post by Archeia, 2021-12-09) | https://forums.rpgmakerweb.com/index.php?threads/rpg-maker-mz-1-4-0-update.142845/ | Loaded (first post only) | Version that added MV-format animations; Effekseer runtime changes |
| Official product page | https://www.rpgmakerweb.com/products/rpg-maker-mz | Loaded | Nothing used beyond marketing claims; no new detail |

No page failed to load. Every help page has a full-context link label, but the fetched text gave it no target, so it was not followed.

---

## 2. Items

## Enemies (ENM)

### RM-ENM-001 — Enemy data: role and list limits
- **What:** An enemy entry describes one foe the party fights. It has stats like an actor, plus its own battle behavior (action patterns).
- **Behavior:**
  - Enemies are one of the Database's list-type data sets. Each entry has a numeric Data ID.
  - The default cap is 2000 enemy entries. The [Change Maximum] button under the list changes it. Lowering the cap deletes every entry above the new limit.
  - The Database overview lists enemies among the data types that have a Notes field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-ENM-002 — Name
- **What:** The enemy's display name.
- **Behavior:**
  - Shown during play. A very long name can be cut off on the play screen.
  - No hard length limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-003 — Image
- **What:** The picture used for the enemy in battle.
- **Behavior:**
  - Double-clicking the image box opens the [Select an Image] window.
  - Choosing [None] gives the enemy no visible image.
  - Asset Standards: front-view enemy images live in `img/enemies` and side-view enemy images in `img/sv_enemies`. Any image size is allowed. An even pixel width is recommended because odd widths may render blurred.
  - How the editor or runtime picks between the two folders (front view vs side view) is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-ENM-004 — Hue
- **What:** A colour shift applied to the enemy image.
- **Behavior:**
  - Set with a [Hue] slider inside the image selection window, so one graphic can be reused in different colors.
  - Numeric range: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-005 — Max HP and Max MP
- **What:** The enemy's maximum HP and MP when battle begins.
- **Behavior:**
  - Max HP: 1 to 999999.
  - Max MP: 0 to 9999.
  - Entered with numeric spin boxes (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img01.png

### RM-ENM-006 — Attack, Defense, M.Attack, M.Defense, Agility, Luck
- **What:** The enemy's six other base parameters at battle start.
- **Behavior:**
  - Each one ranges from 0 to 999.
  - These are fixed per entry. The page describes no level growth for enemies.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-007 — Rewards: EXP
- **What:** Experience the party earns for winning a battle that included this enemy.
- **Behavior:**
  - Range: 1 to 9999999.
  - The page does not say how the EXP is split among party members.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-008 — Rewards: Gold
- **What:** Money the party earns for winning.
- **Behavior:**
  - Range: 0 to 9999999.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-009 — Drop Items
- **What:** Items the party may receive after a victory.
- **Behavior:**
  - A drop can be an item, a weapon or an armor.
  - Double-clicking a drop field opens a window where you pick the item and its [Probability].
  - Probability is entered as "1 in N", from 1/1 (always drops) to 1/1000.
  - The screenshot shows three drop slots, each a button with a "..." picker that reads "None" when empty. The page text does not give the slot count.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img01.png

### RM-ENM-010 — Action Patterns list
- **What:** The list of moves the enemy may use in battle, each with a usage condition and a rating.
- **Behavior:**
  - The list has the columns Skill, Condition and R (rating) (screenshot).
  - Each row opens an [Action] dialog with a "Skill and Rating" group: a Skill picker (the sample shows a skill from the skill database) and a Rating spin box. Below that is a "Conditions" group, followed by OK and Cancel (screenshot).
  - The sample enemy has two rows whose condition reads "Always" with rating 5 (screenshot).
  - Maximum number of rows: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img01.png ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-011 — Rating (priority)
- **What:** A per-action weight that drives which valid action the enemy picks.
- **Behavior:**
  - The text calls it the "priority rating". The dialog labels it "Rating" and the list column "R".
  - See RM-ENM-020 to RM-ENM-023 for how ratings turn into chances.
  - Numeric range and default: not verified. The screenshot only shows the value 5.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-012 — Condition type is a single choice
- **What:** Each action pattern uses exactly one condition type.
- **Behavior:**
  - The Conditions group is a set of radio buttons: Always, Turn, HP, MP, State, Party Level, Switch. The input boxes for a condition are grayed out unless its radio button is selected (screenshot).
  - "Always" is selected in the sample. It appears only in the screenshot, and the page text does not describe it. Its name suggests no restriction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-013 — Condition: Turn
- **What:** Allows the action only on certain turn numbers.
- **Behavior:**
  - Two numbers, A and B, define the turns A + B·X (X = 0, 1, 2 …). A is the first qualifying turn counted from battle start and B is the repeat interval.
  - Example in our own numbers: A = 3 and B = 4 qualifies turns 3, 7, 11 and so on.
  - The dialog shows two spin boxes with "+" between them and "* X" after (screenshot).
  - Allowed values of A and B, and what B = 0 means: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-014 — Condition: HP
- **What:** Allows the action only while the enemy's own HP is in a percentage band.
- **Behavior:**
  - You enter a lower and an upper percentage of Max HP, each from 0 to 100%. The two boxes are joined by "~" (screenshot).
  - Met while current HP falls inside that band.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-015 — Condition: MP
- **What:** Same as the HP condition, but for the enemy's MP as a percentage of Max MP.
- **Behavior:**
  - Lower and upper bound, each from 0 to 100%. Met while current MP is inside the band.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-016 — Condition: State
- **What:** Allows the action only while the enemy has a chosen state.
- **Behavior:**
  - The state is picked from a drop-down list (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-017 — Condition: Party Level
- **What:** Allows the action once the party is strong enough.
- **Behavior:**
  - Met when the highest level among party members is equal to or greater than the number entered. The dialog shows "or above" after the box (screenshot).
  - Allowed range of the number: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-018 — Condition: Switch
- **What:** Allows the action only while a game switch is ON.
- **Behavior:**
  - The switch is picked with a "..." selector (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_06_img02.png

### RM-ENM-019 — Traits
- **What:** Passive properties of the enemy, such as element rates and extra parameters.
- **Behavior:**
  - Double-clicking a row in the Traits list opens the trait editor. The list has the columns Type and Content (screenshot).
  - The editor and the 25 trait types are shared with actors, classes, weapons, armors and states. See How to Set Traits (inventoried in another slice).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-ENM-020 — Action selection, step 1: filter by condition
- **What:** When the enemy acts, only patterns whose condition is currently true are considered.
- **Behavior:**
  - If no pattern qualifies, the enemy does nothing that turn.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-021 — Action selection, step 2: rating window
- **What:** Of the qualifying patterns, only those near the best rating stay in the running.
- **Behavior:**
  - Patterns with the highest rating, and any rated up to 2 points below it, are candidates.
  - Anything 3 or more points below the top rating is excluded.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-022 — Action selection, step 3: relative weights
- **What:** Candidates get weights from their distance to the top rating.
- **Behavior:**
  - Same rating as the top: full weight (call it w).
  - 1 below the top: two-thirds of w.
  - 2 below the top: one-third of w.
  - Candidates with equal ratings get equal chances.
  - Worked example in our own numbers: ratings 9, 8 and 6 leave 6 out (3 below the top). The weights are then 1 and 2/3, giving 60% and 40%.
  - The help's own examples agree. Two equal ratings split 50/50. Ratings top, top−1, top−2 and top−3 give about 50%, 33.3% and 16.6%, and the last is excluded. One top rating with two candidates at top−2 gives 60%, 20% and 20%.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-023 — Action selection, step 4: random draw
- **What:** The action actually used is drawn at random using the weights from step 3.
- **Behavior:**
  - The selection is a weighted random pick over the candidates.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-ENM-024 — Notes
- **What:** Free-text notes for the developer.
- **Behavior:**
  - Has no effect by default, but plugins can read data from it (Database overview).
  - Right-clicking the field shows a menu with [Plugin Help...], which shows plugin help information.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

## Troops & battle events (TRP)

### RM-TRP-001 — Troop data: role and limits
- **What:** A troop is a group of enemies fought together. Encounters on the map and event-started battles both name a troop.
- **Behavior:**
  - A troop is needed even for a fight against a single enemy.
  - Each troop carries its own Battle Events (event pages that run during that battle).
  - Default cap: 2000 troop entries, changeable with [Change Maximum].
  - Troops are not in the overview's list of data types with a Notes field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-TRP-002 — Name
- **What:** The troop's label.
- **Behavior:**
  - Used only in the editor. It has no effect in the game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TRP-003 — Auto-name button
- **What:** Fills in the Name from the enemies currently placed.
- **Behavior:**
  - Clicking [Auto-name] builds a name from the placed enemies.
  - The screenshot's name field reads "Goblin*2", which suggests an "enemy name × count" form. The page text does not state the format, so it is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-004 — Change Battle Background (editor preview only)
- **What:** Chooses the backdrop drawn behind the Placement View.
- **Behavior:**
  - The window has two fields: the left one for the background and the right one for the ground image.
  - Affects only the editor, not the game.
  - The choice is shared with other data editors that show a battle background.
  - The button reads "Change BG..." (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-005 — Battle Test
- **What:** Runs a test battle against this troop straight from the editor.
- **Behavior:**
  - The button reads "Battle Test..." (screenshot).
  - The setup window has four tabs, [1] to [4], for the test party. In each you choose the actor, the level and the equipment.
  - A [Status] area shows the parameters that result from those choices.
  - [OK] opens a game window and starts the battle. The test ends when that window is closed.
  - If plugins are in use, save the project first, or plugin settings will not apply to the test.
  - Exact fields per tab, defaults, and whether the setup is remembered: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-006 — Placement View: membership and positioning
- **What:** A preview area where the troop's enemies are placed.
- **Behavior:**
  - Holds up to 8 enemies. Repeats of the same enemy count toward the 8.
  - Drag an enemy to change where it stands.
  - The enemy database list sits to the right of the buttons and shows ID and name (screenshot).
  - Selecting a placed enemy shows its enemy ID. This was added in MZ to help when typing target IDs into plugin commands.
  - Grid, snapping and coordinate ranges: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-007 — Appear Mid-Battle
- **What:** Marks an enemy as hidden when the battle starts.
- **Behavior:**
  - Right-click a placed enemy and choose [Appear Mid-Battle].
  - The enemy stays absent until a battle event runs the [Enemy Appearance] event command.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TRP-008 — Add
- **What:** Puts the selected enemy from the list into the troop.
- **Behavior:**
  - Click the button (labeled "< Add" in the screenshot) or double-click an enemy in the list.
  - The order enemies are added in becomes the order of the enemy target list in battle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-009 — Delete (Remove)
- **What:** Takes the selected placed enemy out of the troop.
- **Behavior:**
  - The help text calls it "Delete". The screenshot button reads "Remove >".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-010 — Clear
- **What:** Empties the troop.
- **Behavior:**
  - Removes every placed enemy at once.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TRP-011 — Align
- **What:** Tidies enemy positions automatically.
- **Behavior:**
  - Lays the placed enemies out in a row from left to right, in the order they were added.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TRP-012 — Battle Event pages and page buttons
- **What:** Each troop has a tabbed set of event pages. These are the battle events, and they work like map event pages.
- **Behavior:**
  - The buttons on the left are New Event Page, Copy Event Page, Paste Event Page and Clear Event Page. The screenshot also shows a Delete Event Page button.
  - From the map-event page these buttons share behavior with: New adds a page after the current one. Copy puts the current page on the clipboard. Paste inserts the clipboard page after the current one under a new number. Delete removes the current page and renumbers the pages after it down by one. Clear resets the current page to empty.
  - Pages show as numbered tabs, and a new troop starts with page "1" (screenshot).
  - Maximum page count: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-013 — Contents
- **What:** The event command list run when a page's conditions and span allow it.
- **Behavior:**
  - Edited exactly like map event contents.
  - ◆ rows are commands. Double-click to add or insert one.
  - The right-click menu offers New, Edit, Cut, Copy, Paste, Delete, Select All, Copy as Text, Copy as HTML Tags and Test.
  - Shift-click selects a range, but only within the same branch.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-TRP-014 — Conditions: dialog and page-selection rules
- **What:** Decides when a battle event page may run.
- **Behavior:**
  - Opened with the [...] button next to "Conditions". A new page's summary reads "Don't Run" (screenshot).
  - Five condition types can be enabled: Turn End, Turn, Enemy HP, Actor HP and Switch. Each has its own checkbox, so several can be ticked at once (screenshot).
  - Unlike map events, a page with no condition enabled never runs.
  - If several pages qualify, the lowest-numbered page is used.
  - Whether multiple ticked conditions combine as AND or OR: not verified for battle events.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-015 — Condition: Turn End
- **What:** Ties the page to the end of a turn.
- **Behavior:**
  - A checkbox with no further parameters (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-016 — Condition: Turn
- **What:** Ties the page to specific turn numbers counted from battle start.
- **Behavior:**
  - The left box is the starting turn and the right box is the repeat interval.
  - The dialog lays this out as "[ ] + [ ] * X", the same pattern as the enemy Turn condition (screenshot).
  - Allowed values: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-017 — Condition: Enemy HP
- **What:** Fires when a chosen troop enemy's HP drops to a percentage of its Max HP.
- **Behavior:**
  - Pick the enemy from a drop-down and enter a percentage.
  - The text says "falling below". The dialog shows "or below" after the value box (screenshot).
  - How enemies are listed (for example by troop slot): not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-018 — Condition: Actor HP
- **What:** Fires when a chosen actor's HP drops to a percentage of Max HP.
- **Behavior:**
  - Pick the actor from a drop-down and enter a percentage. The dialog shows "or below" (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-019 — Condition: Switch
- **What:** Fires while a chosen switch is ON.
- **Behavior:**
  - The switch is chosen with a "..." selector (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img02.png

### RM-TRP-020 — Span: Battle
- **What:** The page runs at most once per battle.
- **Behavior:**
  - Runs the first time its conditions are met and never again in that battle.
  - It is the value shown in the Span drop-down of a fresh page (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_07_img01.png

### RM-TRP-021 — Span: Turn
- **What:** The page can run once per turn.
- **Behavior:**
  - Conditions are checked every turn, and the page runs in any turn where they hold.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-TRP-022 — Span: Moment
- **What:** The page runs again and again while its conditions hold.
- **Behavior:**
  - The help warns that without switch control a Moment page can stop the battle from advancing.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

## States (STA)

### RM-STA-001 — State data: role and limits
- **What:** States are conditions such as poison or confusion that change a battler's stats or behavior.
- **Behavior:**
  - States can be harmful (for example HP loss over time) or helpful (for example a berserk-style attack boost).
  - States fall under the overview's general cap of 1000 entries, changeable with [Change Maximum].
  - States have a Notes field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-STA-002 — State #1 is Knockout
- **What:** The first state is reserved as the death or KO state.
- **Behavior:**
  - The engine applies it automatically to any actor or enemy whose HP reaches 0.
  - If every party member has it, the game is over.
  - The Traits page adds a related detail: a State Resist trait on Knockout keeps a character from being knocked out at 0 HP.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-STA-003 — Name
- **What:** The state's name.
- **Behavior:**
  - The help says the name is never used in gameplay, because states are shown as icons.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-004 — Icon
- **What:** The icon that marks an affected battler.
- **Behavior:**
  - Party members display it beside their name; enemies display it over the sprite.
  - Chosen through the [Select an Image] window (double-click). The editor shows the chosen icon's index number under it (screenshot).
  - Icons come from the system icon sheet (IconSet.png in `img/system`, per Asset Standards).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-005 — Restriction options
- **What:** Limits on what a battler with this state can do.
- **Behavior:**
  - There are five choices, in this list order: None, Attack an Enemy, Attack Anyone, Attack an Ally, Cannot Move.
  - None: no limit.
  - Attack an Enemy: an actor is forced to attack an enemy.
  - Attack Anyone: an actor is forced to attack either side.
  - Attack an Ally: an actor is forced to attack an ally.
  - Cannot Move: the battler cannot act at all.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-006 — Restriction precedence
- **What:** How to resolve several restricting states at once.
- **Behavior:**
  - The restriction that sits lower in the option list wins, so Cannot Move beats Attack an Ally, which beats Attack Anyone, and so on.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-007 — Priority
- **What:** Decides which state icon is shown when a battler has several states.
- **Behavior:**
  - Range: 0 to 100.
  - The icon of the state with the highest priority is displayed.
  - On a tie, the state with the lowest ID wins.
  - The help states no other role for this field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-008 — [SV] Motion
- **What:** The side-view battler pose used while the state is active.
- **Behavior:**
  - Applies only in side-view battles.
  - Chosen from a drop-down. The Knockout sample shows "Dead" (screenshot).
  - The side-view sheet standard names 18 motions, including Status Ailment, Sleep, Dead and Near-Death. Which of them this drop-down offers is not verified.
  - Sheet format (for context): one file per character, a 9×6 grid of 54 frames, 576×384 at standard size. Three frames in a row form one motion. Looping idle motions play 1-2-3-2 and one-shot motions play 1-2-3.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_11_02_img02.png

### RM-STA-009 — [SV] Overlay
- **What:** An animated overlay drawn on a side-view battler while the state is active.
- **Behavior:**
  - Applies only in side-view battles.
  - Chosen from a drop-down. The sample shows "None" (screenshot).
  - Asset Standards names `img/system/States.png` as the asset for ally states in side-view battle.
  - Option list: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-010 — Removal Conditions: overview
- **What:** A set of independent rules for clearing the state.
- **Behavior:**
  - Each enabled rule is checked on its own, and any one that triggers removes the state.
  - Remove at Battle End, Remove by Restriction, Remove by Damage and Remove by Walking are checkboxes. Auto-removal Timing is a drop-down (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-011 — Remove at Battle End
- **What:** Clears the state when the battle ends.
- **Behavior:**
  - A checkbox.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-012 — Remove by Restriction
- **What:** Clears the state when a different restricting state is applied.
- **Behavior:**
  - When enabled, adding another state whose restriction differs removes this one.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-STA-013 — Auto-removal Timing
- **What:** When the turn countdown is checked.
- **Behavior:**
  - Action End: checked after the affected battler finishes an action.
  - Turn End: checked after the turn ends, before the next action selection.
  - The drop-down also has "None", the value shown in the sample (screenshot). With None selected, the Duration in Turns boxes are grayed out (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-014 — Duration in Turns
- **What:** How many turns the state lasts before auto-removal.
- **Behavior:**
  - Two boxes, a lower limit and an upper limit, joined by "~" (screenshot). Each ranges from 0 to 9999 turns.
  - The count starts when the state is applied.
  - Whether the actual length is picked at random between the two limits: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-015 — Remove by Damage
- **What:** A chance to clear the state whenever the battler takes damage.
- **Behavior:**
  - Removal chance: 0 to 100%.
  - Rolled on any kind of damage.
  - A checkbox with a value box (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-016 — Remove by Walking
- **What:** Clears the state after the player walks a distance on the map.
- **Behavior:**
  - Distance: 0 to 9999.
  - The text counts it in tiles moved. The editor label after the box reads "steps" (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-017 — Messages
- **What:** Battle log lines tied to the state.
- **Behavior:**
  - Four text slots, one per situation: the state lands on a party member; it lands on an enemy; it is still active; it wears off.
  - The token %1 is replaced by the affected battler's name.
  - The editor heading reads "Messages" while the help heading says "Message" (screenshot).
  - When the "persists" line is shown: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_08_img01.png

### RM-STA-018 — Traits
- **What:** Properties granted to the battler while the state is on them.
- **Behavior:**
  - A state's traits are merged into the battler's traits for as long as the state lasts.
  - It uses the same trait editor and the same 25 trait types as other data. See How to Set Traits (other slice).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-STA-019 — Notes
- **What:** Free-text notes field.
- **Behavior:**
  - Has no effect by default, but plugins can read it.
  - Right-clicking offers [Plugin Help...].
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

## Animations & Effekseer (ANI)

### RM-ANI-001 — Animation data: role and where it is used
- **What:** Animations are visual effects played in battle (for example on a hit) or on the map.
- **Behavior:**
  - MZ's animation editor was rebuilt around 3D particle effects ("effects"). You pick an effect and then set its sound and flash.
  - Items, skills, weapons and the Show Animation event command choose from this list.
  - Animations fall under the general cap of 1000 entries, changeable with [Change Maximum].
  - Animations are not in the overview's list of data types with a Notes field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-ANI-002 — Name
- **What:** The animation's label.
- **Behavior:**
  - Used only in the editor. It has no effect in the game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html

### RM-ANI-003 — Display Type
- **What:** Where on screen the animation is placed in the game.
- **Behavior:**
  - A drop-down. The only value seen is "For each target" (screenshot).
  - The other options and what they do are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-004 — Particle Effect: File Name
- **What:** The Effekseer effect file the animation plays.
- **Behavior:**
  - Picked with a "..." selector labeled "Filename" (screenshot).
  - Files come from the project's `effects` folder (see RM-ANI-012).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_09_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-005 — Scale
- **What:** The size of the effect.
- **Behavior:**
  - A spin box shown in percent. The sample reads "50 %" (screenshot).
  - Range and default: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-006 — Speed
- **What:** The effect's playback speed.
- **Behavior:**
  - A spin box shown in percent. The sample reads "100 %" (screenshot).
  - Range and default: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-007 — Rotation
- **What:** The effect's rotation angle.
- **Behavior:**
  - The field is a "..." button whose summary reads "X:0, Y:0, Z:0", which indicates rotation on three axes set in a sub-dialog (screenshot).
  - Units and range: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-008 — Offset X and Offset Y
- **What:** Shift the animation's position.
- **Behavior:**
  - Values are in pixels. X moves the animation horizontally and Y vertically.
  - The sample shows 0 for both (screenshot).
  - Range and sign convention: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-009 — Preview
- **What:** An in-editor viewer for testing the animation.
- **Behavior:**
  - Play: runs the animation in the preview.
  - Change Target: swaps the target image shown. The button reads "Change Target..." (screenshot).
  - Change Battle Background: swaps the backdrop used for testing. The button reads "Change Battleback..." (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-010 — Sounds (timed sound effects)
- **What:** Sound effects played at chosen frames of the animation.
- **Behavior:**
  - The list has the columns Frame and Filename. Frames show as "#001" and so on (screenshot).
  - Each row plays its sound file on its frame.
  - Volume, pitch, pan and maximum rows: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-011 — Flash (timed flashes)
- **What:** Colour flashes played at chosen frames.
- **Behavior:**
  - The list has the columns Frame, Duration and Color (screenshot).
  - Frame: when the flash starts.
  - Duration: how long it lasts, in 1/60-second units.
  - Color: a four-number value. The sample is (255,255,255,255), which reads as red, green, blue plus a fourth channel (screenshot). What the fourth channel means is not stated.
  - The help and screenshot show a single Flash list. Separate target flash, screen flash or "hide target" entries are not verified for the Effekseer editor.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_09_img01.png

### RM-ANI-012 — Effekseer: purpose and import workflow
- **What:** Effekseer is the separate effect-authoring tool used to make custom particle effects for MZ animations.
- **Behavior:**
  - It is meant for particle effects such as explosions and hit sparks, and for editing them easily.
  - The help says MZ officially supports Effekseer version 1.5.
  - Export effects in `.efkefc` format and place them in the project's `effects` folder. They then appear for selection under Particle Effect in Database → Animations.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09_01.html

### RM-ANI-013 — Effekseer: samples and documentation
- **What:** Resources the help points to.
- **Behavior:**
  - The Effekseer project publishes many user-submitted sample effects, mostly under CC-0, which can be used as-is or adapted.
  - The help links to the Effekseer site (https://effekseer.github.io/en/), its samples page (https://effekseer.github.io/en/contribute.html) and its tutorial help (https://effekseer.github.io/Help_Tool/en/index.html).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_09_01.html

### RM-ANI-014 — Effekseer runtime changes in MZ 1.4.0
- **What:** Engine-level behavior of Effekseer animations announced in the 1.4.0 update.
- **Behavior:**
  - The bundled Effekseer version was updated to improve rendering performance.
  - Several animations can now play at the same time on mobile.
  - Effekseer animations pause while the game window is not active.
- **Source:** https://forums.rpgmakerweb.com/index.php?threads/rpg-maker-mz-1-4-0-update.142845/

### RM-ANI-015 — MV animations: help's conversion guidance
- **What:** How the help treats MV-format animation data when converting an MV project.
- **Behavior:**
  - The MV-to-MZ conversion table lists Animations (`data/Animations.json` plus `img/animations`) as "Not recommended", because, it says, they cannot be edited in MZ.
  - If you convert them anyway, copy the `img/animations` folder by hand, since MZ projects have none by default.
  - This page appears to predate the 1.4.0 change in RM-ANI-016. The two official sources disagree on editability.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-ANI-016 — MV-format (spritesheet) animations added in 1.4.0
- **What:** MZ 1.4.0 added the option to create cell or spritesheet animations in the same format as RPG Maker MV, alongside Effekseer ones.
- **Behavior:**
  - Announced as a major item of the 1.4.0 update.
  - The official blog describes it as the choice for people who prefer cell-based animation to Effekseer.
- **Source:** https://forums.rpgmakerweb.com/index.php?threads/rpg-maker-mz-1-4-0-update.142845/ ; https://www.rpgmakerweb.com/blog/making-mv-style-animations

### RM-ANI-017 — MV-format animations: setup and sheet layout
- **What:** How to start an MV-style animation in MZ.
- **Behavior:**
  - Create an `animations` folder inside the project's `img` folder and put the animation sheets there.
  - A sheet is rows of 5 cells, each 192×192 px. Up to 20 rows (100 cells) are allowed.
  - In the Animations list, right-click an empty entry and choose "create MV-compatible data" to get the MV-style editor for that entry.
  - Hovering over each section of that editor shows a short explanation.
- **Source:** https://www.rpgmakerweb.com/blog/making-mv-style-animations

### RM-ANI-018 — MV-format editor: frames and cells
- **What:** The frame and cell editing model of the MV-style editor.
- **Behavior:**
  - A new MV-style animation starts with 1 frame. Max Frames sets the total.
  - The Frame List supports copying and pasting frames from the right-click menu.
  - Each frame holds cells. Right-click a cell and choose Edit to open Cell Properties, which include Pattern (which sheet cell), X, Y and Scale. The blog uses 100% and 500% scale values.
  - A Position setting anchors the animation. "feet" is one option. Others are not verified.
  - A "change target" button previews the animation on different enemies.
- **Source:** https://www.rpgmakerweb.com/blog/making-mv-style-animations

### RM-ANI-019 — MV-format editor: Batch, Shift, Tween
- **What:** Bulk-editing tools in the MV-style editor.
- **Behavior:**
  - Batch: changes cells across a range of frames and a range of cell indices at once, for example the X and Y of one cell over every frame.
  - Shift: nudges cells around until they sit where you want.
  - Tween: fills the frames between a start frame and an end frame by interpolating cell properties over a chosen frame range and cell range. Checkboxes pick which properties stay unchanged, and all are ticked by default in the blog's example. The blog warns that tweening can switch a pattern a frame early, so check the frames afterwards.
- **Source:** https://www.rpgmakerweb.com/blog/making-mv-style-animations

### RM-ANI-020 — MV-format editor: SE and Flash Timing
- **What:** Timed sounds and flashes for MV-style animations.
- **Behavior:**
  - A section named "SE and Flash Timing" sets which frames play a sound effect or a flash. The blog's example flashes the enemy (target) twice and plays one sound.
  - Field details (flash scope such as target, screen or hide target; colour; duration; SE volume and pitch): not verified.
- **Source:** https://www.rpgmakerweb.com/blog/making-mv-style-animations

## Common Events (CEV)

### RM-CEV-001 — Common event data: role, invocation and limits
- **What:** Common events are reusable event scripts that any part of the game can run.
- **Behavior:**
  - Uses include background monitors that check game state over and over, and routines that run when an item or skill is used.
  - They are started by the [Common Event] event command, by turning a chosen switch ON (Autorun or Parallel trigger), or from skill or item effects.
  - Since MZ, if a skill or item has several common-event effects, all of them run. Before, only one could run.
  - Common events fall under the general cap of 1000 entries, changeable with [Change Maximum].
  - Common events are not in the overview's list of data types with a Notes field.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-CEV-002 — Name
- **What:** The common event's label.
- **Behavior:**
  - Used only in the editor. It has no effect in the game.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html

### RM-CEV-003 — Trigger: None
- **What:** The common event runs only when something calls it.
- **Behavior:**
  - Run only by an explicit call, such as a skill, an item or another event.
  - It is the value shown in the Trigger drop-down of a blank entry (screenshot).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_11_img01.png

### RM-CEV-004 — Trigger: Autorun
- **What:** The common event starts on its own when its switch is ON.
- **Behavior:**
  - Runs once the chosen [Switch] turns ON, and keeps repeating while it stays ON (see RM-CEV-008).
  - Only active while the map screen is shown.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html

### RM-CEV-005 — Trigger: Parallel
- **What:** The common event runs in the background, on repeat, while its switch is ON.
- **Behavior:**
  - Runs periodically while the chosen [Switch] is ON.
  - Only active while the map screen is shown.
  - Whether it runs during battle is not stated. The help only restricts it to the map screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html

### RM-CEV-006 — Switch
- **What:** The game switch that starts an Autorun or Parallel common event.
- **Behavior:**
  - Used only when Trigger is Autorun or Parallel. In the screenshot, with Trigger = None, the Switch label and selector appear grayed out (screenshot).
  - If several common events share the same switch, only the one with the lowest ID (highest in the list) runs.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/assets/img/01_08_11_img01.png

### RM-CEV-007 — Contents
- **What:** The event command list the common event runs.
- **Behavior:**
  - Edited the same way as map event contents (◆ rows, double-click to add or insert, and the right-click menu New, Edit, Cut, Copy, Paste, Delete, Select All, Copy as Text, Copy as HTML Tags, Test).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html

### RM-CEV-008 — Autorun and Parallel loop warning
- **What:** These common events repeat for as long as their switch stays ON.
- **Behavior:**
  - Authors must turn the switch OFF somewhere (usually inside the event) to end the loop.
  - An uncontrolled loop can leave the player unable to control the game.
  - If that happens in a playtest, close the game window with its [X] button or with Alt+F4.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_11.html

---

## 3. Not verified

The fetched official pages do not state the following. They need another official source, or a decision made without RPG Maker as a reference.

**Enemies**
- Numeric range and default of an action pattern's Rating.
- Maximum number of action patterns per enemy.
- Allowed values of Turn A and B, and what B = 0 means.
- Allowed range of the Party Level value.
- Numeric range of the enemy image Hue.
- Number of drop item slots (three seen only in the screenshot, not stated in the text).
- Behavior of the "Always" condition (seen only in the screenshot).
- How the image is chosen between `img/enemies` and `img/sv_enemies` (front view vs side view).
- How EXP is split among party members.
- How the MP condition behaves for an enemy with Max MP 0.
- Any name length limit.

**Troops and battle events**
- Exact fields, defaults and memory of the Battle Test window tabs [1] to [4].
- Format of [Auto-name] output, and how duplicates are named (screenshot suggests "Name*count").
- Maximum number of battle event pages.
- Whether several ticked conditions on one page combine as AND or OR.
- Allowed values for the Turn condition and whether turn 0 exists.
- Whether Enemy HP or Actor HP fires at "below" or "at or below" (the text and the dialog differ).
- How enemies are listed in the Enemy HP drop-down.
- Placement grid, snapping, coordinate range, and whether front and side views place enemies differently.

**States**
- List of [SV] Motion options.
- List of [SV] Overlay options.
- Whether the actual duration is random between the Duration in Turns lower and upper limits.
- The "None" Auto-removal Timing option (seen only in the screenshot) and its exact semantics.
- When the "If the state persists" message is shown.
- Whether Remove by Walking counts steps or tiles in edge cases (the text says tiles, the UI says steps).

**Animations**
- Display Type options other than "For each target", and their behavior.
- Ranges, defaults and units of Scale, Speed, Rotation and Offset X/Y (only sample values were seen).
- Whether the Effekseer editor has separate target flash, screen flash or hide-target flash entries.
- Meaning of the fourth number in the flash Color.
- Sound volume, pitch and pan, and maximum rows.
- Frame rate of animation frames (only the flash Duration unit of 1/60 s is stated).
- MV-format editor details: Position options other than "feet", flash scope, hide target, hue.
- Which source is current on MV-format animations. The help's MV conversion page says they cannot be edited in MZ, while the 1.4.0 release post and the official blog say MV-format animations can be created. The help page probably predates 1.4.0, but that is also not verified.
- Whether Effekseer versions other than 1.5 work.

**Common Events**
- Whether Autorun or Parallel common events run during battle (the help only says they need the map screen).
- Whether the Switch field is formally disabled for Trigger None (seen grayed out in the screenshot only).
- Any per-type entry limit beyond the general 1000 default.
