# RPG Maker MZ feature inventory: Database part C (System 1/2, Types, Terms, Traits, Effects)

Clean-room research notes for Talewright. Every item below is written in our own words from official RPG Maker MZ help pages fetched on 2026-10-13. The help text was not copied; field names and UI labels are given as-is. Default text strings from the Terms screen are intentionally left out; only the slot names are listed.

## 1. Pages covered

Base: `https://rpgmakerofficial.com/product/MZ_help-en/`

Each primary page was fetched with WebFetch (all loaded). The WebFetch summarizer only gave a condensed version, so the same official URLs were also downloaded as raw HTML (HTTP 200) to read the full text. The screenshots embedded in each page were downloaded from the help's own `assets/img/` folder and inspected for field labels. Facts that come only from a screenshot are marked **(screenshot)**. Values in a screenshot are sample-project values, not documented defaults.

| Page | URL | Status |
|---|---|---|
| System 1 Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html | Loaded |
| System 2 Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html | Loaded (short page: it documents only Menu Commands, Item Categories, [SV] Attack Motions, [SV] Magic Skills and Advanced Settings) |
| Type Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html | Loaded |
| Terms Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html | Loaded (gives groups only; slot names come from its screenshot) |
| How to Set Traits | https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html | Loaded (25 trait types) |
| How to Set Effects | https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html | Loaded (13 effect types) |

Supplementary official pages, followed from in-help links or from the "New Functions" pages that point back to System 1/2:

| Page | URL | Status |
|---|---|---|
| Database (index) | https://rpgmakerofficial.com/product/MZ_help-en/01_08.html | Loaded |
| Improvements to Artistic Expressions in Games (Attack Skills, System split, Common Events) | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html | Loaded (linked from the Traits page as `01_01_01_05.html#m01`) |
| Main Features of RPG Maker MZ | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html | Loaded |
| MV Data Conversion (font note) | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html | Loaded |
| Time Progression Battles | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html | Loaded |
| Game Control Improvements (Autosave, Game ID) | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html | Loaded |
| 01_01_01.html, 01_01_01_07.html | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01.html | 404. These were guessed index URLs, not linked from any page. Nothing depends on them. |

Screenshots inspected (all HTTP 200):
- `assets/img/01_08_12_img01.png`: System 1 screen
- `assets/img/01_08_12_img02.png`: title image picker
- `assets/img/01_08_12_img03.png`: System 2 screen
- `assets/img/01_08_13_img01.png`: Types screen
- `assets/img/01_08_14_img01.png`: Terms screen
- `assets/img/01_08_15_img01.png`: Traits dialog
- `assets/img/01_08_16_img01.png`: Effects dialog

All are under `https://rpgmakerofficial.com/product/MZ_help-en/`.

---

## 2. Items

### SYS: System 1 and System 2

### RM-SYS-001 — System settings split into two pages
- **What:** The project-wide system configuration is spread over two database tabs, System 1 and System 2.
- **Behavior:**
  - System 1 holds the game's starting state and global options: party, start positions, the music and sounds used in each situation, and similar settings.
  - System 2 holds customization options plus "Advanced Settings" such as resolution and fonts.
  - MZ introduced the split. It also added title command-window settings, a choice of battle system (time progression), three new options (Show TP in Window, Show Key Item quantities, Enable Autosave) and the Advanced Settings block.
  - System, Types and Terms are single configuration screens, not lists of numbered entries like the other database tabs.
  - On disk all of these live in one data file: the MV-conversion page maps "System / Vocabulary / Types / Switch names / Variable names" to `data/System.json`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-SYS-002 — Game Title
- **What:** The game's name.
- **Behavior:**
  - It starts with the name typed when the project was created.
  - It is shown as the game's title and in the window title bar.
  - The Title Screen can optionally draw it on screen (see RM-SYS-007).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-003 — Starting Party
- **What:** The list of actors who are in the party when a new game begins.
- **Behavior:**
  - The editor sets no limit on how many actors the list can hold.
  - Only the first four actors take part in battle.
  - Double-click an entry, or a blank row, to choose or add an actor. Right-click an entry and choose Delete to remove it.
  - **(screenshot)** It is shown as a one-column "Actor" list.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-004 — Currency
- **What:** The name of the money unit.
- **Behavior:**
  - It is a free-text name.
  - It is used wherever the party's money is displayed, for example the gold readout on the Menu Screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-005 — Window Color
- **What:** A tint applied to the background of every in-game window.
- **Behavior:**
  - It is set as three components, Red, Green and Blue, each from -255 to 255.
  - Double-clicking the field opens the editor.
  - **(screenshot)** The field shows a preview swatch.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-006 — Vehicle Images
- **What:** The map sprites used for the three vehicles: Boat, Ship and Airship.
- **Behavior:**
  - Each vehicle has its own image field. Double-click it to pick an image.
  - Choosing "(None)" means the vehicle shows no image.
  - **(screenshot)** There are three side-by-side preview boxes labeled Boat, Ship and Airship.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-007 — Starting Positions (player, boat, ship, airship)
- **What:** The map and tile where the player and each vehicle appear when a new game starts.
- **Behavior:**
  - There are four fields: Player, Boat, Ship and Airship.
  - Each field has a "..." button that opens a picker. Click a tile on the map preview to place the start point.
  - The start point is drawn on the map as an icon with a blue frame. You can drag it like a map event, and the Del key removes it.
  - A player start position is required. Without one the game cannot be started.
  - **(screenshot)** The Player field shows the map name with (x,y) coordinates. The vehicle fields can be set to "None", so vehicle start positions are optional.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-008 — Title Screen image(s) and Draw Game Title
- **What:** The artwork shown on the first screen after launch, plus a switch that overlays the game title.
- **Behavior:**
  - The "..." button opens a "Select an Image" picker.
  - When "Draw Game Title" is checked, the Game Title text is drawn near the top of the title screen. Uncheck it when the artwork already contains the logo.
  - **(screenshot)** The field is labeled "Images:". The picker shows two separate file lists side by side, each with a "(None)" choice, plus a preview pane and OK/Cancel. This suggests a two-layer title image, but the page text does not explain the two lists.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-009 — Title Command Window
- **What:** Settings for the window that holds the title-screen commands.
- **Behavior:**
  - It sets the window's background type and its position.
  - The feature was added in MZ.
  - **(screenshot)** The field reads "Window (X+0, Y+0)", which suggests a background type plus an X/Y offset, and has a "..." button.
  - The available background types and offset ranges are not documented (see the Not verified list).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-SYS-010 — Battle Screen (view type)
- **What:** Chooses the camera viewpoint for battles.
- **Behavior:**
  - There are two radio options.
  - **Front-view:** the enemies are shown facing the camera.
  - **Side-view:** both enemies and actors are shown from the side.
  - Side-view uses the [SV] settings in System 2 (RM-SYS-022 and RM-SYS-023).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-011 — Battle System
- **What:** Chooses the battle flow model.
- **Behavior:**
  - There are three radio options. The screenshot labels them "Turn-based", "Time Progress (Active)" and "Time Progress (Wait)".
  - **Turn-Based:** every battler's actions in a round are resolved together.
  - **Time Progression (Active):** turn order depends on agility, and the clock keeps running while the player picks commands.
  - **Time Progression (Wait):** turn order depends on agility, and the clock pauses while the player picks commands.
  - How Time Progression Battles (TPB) work:
    - Each battler has a gauge that fills gradually. The English page calls it a "TP gauge".
    - A battler whose gauge reaches 100% chooses an action.
    - After acting, the gauge drops to 0 and starts filling again.
  - TPB is new in MZ.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html

### RM-SYS-012 — Option: Start Transparent
- **What:** A checkbox that starts the game with the player sprite invisible.
- **Behavior:**
  - When checked, the player is hidden at game start.
  - The "Change Transparency" event command makes the player visible again.
  - **(screenshot)** The checkbox is labeled "Start in Transparent".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-013 — Option: Show Player Followers
- **What:** A checkbox that starts the game with party members trailing the leader on the map.
- **Behavior:**
  - When checked, the members after the leader walk behind the lead actor.
  - At most four actors are drawn on the map, even when the party has five or more.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-014 — Option: Death by Slip Damage
- **What:** Controls whether damage-over-time from states (Poison and similar) can kill.
- **Behavior:**
  - When checked, slip damage can bring HP to 0.
  - When unchecked, slip damage stops at 1 HP.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-015 — Option: Death by Floor Damage
- **What:** Controls whether damage-floor tiles on the map can kill.
- **Behavior:**
  - When checked, floor damage can bring HP to 0.
  - When unchecked, floor damage stops at 1 HP.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-016 — Option: Show TP in Window
- **What:** Shows or hides each member's TP in the status display.
- **Behavior:**
  - When checked, TP appears for every party member in the Status Window.
  - Uncheck it in games that don't use TP.
  - It was added in MZ.
  - **(screenshot)** The checkbox is labeled "Display TP in Window".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-SYS-017 — Option: EXP for Reserve Members
- **What:** Gives battle experience to party members who didn't fight.
- **Behavior:**
  - When checked, members outside the battle group also receive EXP when a battle is won.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-018 — Option: Show Key Item Quantities
- **What:** Shows how many of each Key Item the party holds.
- **Behavior:**
  - When checked, the Item screen displays held counts for Key Items.
  - It was added in MZ.
  - **(screenshot)** The checkbox is labeled "Show the Number of Key Items".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-SYS-019 — Option: Enable Autosave
- **What:** Turns on automatic saving.
- **Behavior:**
  - When checked, the game saves automatically when a battle ends and when the player moves between locations (player transfer).
  - The first (topmost) save slot is reserved as the autosave slot.
  - Players can resume from the autosave if a session is interrupted.
  - Unchecking the option disables autosave.
  - It was added in MZ.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html

### RM-SYS-020 — Music (system BGM / ME slots)
- **What:** The audio played in fixed system situations.
- **Behavior:**
  - Each slot takes one BGM or ME file.
  - **Title** (BGM): plays on the title screen.
  - **Battle** (BGM): plays during battle.
  - **Victory** (ME): plays when the party wins.
  - **Defeat** (ME): plays when the party loses.
  - **Game Over** (ME): plays on the game-over screen.
  - **Boat** (BGM): plays while riding the boat.
  - **Ship** (BGM): plays while riding the ship.
  - **Airship** (BGM): plays while riding the airship.
  - Normal map music is not set here; it is part of each map's settings.
  - **(screenshot)** The slots are shown as a Type / Filename table.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-021 — Sounds (system SE slots)
- **What:** A table of 24 sound-effect slots for UI and battle events.
- **Behavior:** Each slot takes one SE file.
  - **Cursor:** cursor movement.
  - **OK:** confirming a command.
  - **Cancel:** backing out, for example in the menu.
  - **Buzzer:** trying to pick something that can't be used.
  - **Equip:** changing equipment in the menu.
  - **Save:** saving the game.
  - **Load:** loading a save.
  - **Battle Start:** an encounter begins.
  - **Escape:** the party or an enemy runs from battle.
  - **Enemy Attack:** an enemy uses a normal attack.
  - **Enemy Damage:** an enemy takes damage.
  - **Enemy Collapse:** an enemy is defeated.
  - **Boss Collapse 1:** a defeated enemy has the Collapse Effect trait set to Boss.
  - **Boss Collapse 2:** plays during that boss enemy's vanishing effect.
  - **Ally Damage:** an actor takes damage. The screenshot labels this slot "Actor Damage".
  - **Ally Death:** an actor is knocked out in battle. The screenshot labels this slot "Actor Collapse".
  - **Recovery:** an actor or enemy is healed in battle.
  - **Miss:** a physical attack misses.
  - **Evasion:** a physical attack is evaded.
  - **Magic Evasion:** a magic attack is evaded.
  - **Magic Reflection:** a magic attack is reflected.
  - **Shop:** buying or selling in a shop.
  - **Use Item:** using an item from the menu.
  - **Use Skill:** using a skill from the menu.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-SYS-022 — [SV] Attack Motions (System 2)
- **What:** Sets which side-view battler motion, and which weapon graphic, is used for a normal attack with each weapon type.
- **Behavior:**
  - It applies to side-view battles only.
  - **(screenshot)** The table has three columns: Type, Motion and Image.
  - **(screenshot)** The rows are "Bare Hands" plus one row per weapon type from Types.
  - **(screenshot)** The motion values seen are Thrust, Swing and Missile.
  - **(screenshot)** The Image column chooses a weapon graphic or None.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-023 — [SV] Magic Skills (System 2)
- **What:** The skill types that count as "magic" for side-view animation.
- **Behavior:**
  - Skills of the listed types use the casting motion in side-view.
  - **(screenshot)** It is a one-column list of skill types.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-024 — Menu Commands (System 2)
- **What:** Turns individual main-menu commands on or off.
- **Behavior:**
  - Each command has an ON/OFF checkbox.
  - **(screenshot)** The checkboxes are Item, Skill, Equip, Status, Formation and Save.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-025 — Item Categories (System 2)
- **What:** Turns individual categories on the Item screen on or off.
- **Behavior:**
  - Each category has an ON/OFF checkbox.
  - **(screenshot)** The checkboxes are Item, Weapon, Armor and Key Item.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-026 — Advanced Settings: Game ID
- **What:** A numeric identifier for the project.
- **Behavior:**
  - The ID can be edited.
  - It tells one game apart from another. It lets save data carry over when a browser-published game is updated, even if the title has changed.
  - The ID is shown in System 2 under Advanced Settings.
  - **(screenshot)** It is shown as a Name/Value row containing an 8-digit number.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html

### RM-SYS-027 — Advanced Settings: Screen Width / Screen Height
- **What:** The game's rendering resolution.
- **Behavior:**
  - Width and height are separate fields.
  - Being able to set the resolution in the editor is new in MZ.
  - **(screenshot)** The sample project shows 816 × 624.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-SYS-028 — Advanced Settings: UI Area Width / UI Area Height
- **What:** The size of the region the user interface is laid out in, separate from the screen size.
- **Behavior:**
  - Width and height are separate fields.
  - **(screenshot)** The sample project shows 816 × 624, the same as the screen size there.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-029 — Advanced Settings: Main Font Filename
- **What:** The font file used for general game text.
- **Behavior:**
  - In MZ, fonts are chosen in the database rather than in a stylesheet. For MV projects being converted, only the font file itself should be copied over.
  - **(screenshot)** The value is a `.woff` filename.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html

### RM-SYS-030 — Advanced Settings: Number Font Filename
- **What:** A separate font file used when drawing numbers.
- **Behavior:**
  - **(screenshot)** The value is a `.woff` filename.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-031 — Advanced Settings: Fallback Font
- **What:** The font used for any character the main font doesn't contain.
- **Behavior:**
  - **(screenshot)** It is labeled "Fallback Fonts". The value is a comma-separated, CSS-style font-family list naming a system font followed by a generic family.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

### RM-SYS-032 — Advanced Settings: Font Size
- **What:** The base text size for the game.
- **Behavior:**
  - **(screenshot)** The sample project shows 26.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html

---

### TYP: Types

### RM-TYP-001 — Type name lists (shared editing model)
- **What:** One screen holds five editable name lists: Elements, Skill Types, Weapon Types, Armor Types and Equipment Types. Other database screens pick from these names.
- **Behavior:**
  - Each list is numbered by ID. **(screenshot)** IDs are shown as 01, 02, ...
  - To rename an entry, select it and type the new name in the text field under the list.
  - Each list has a "Change Maximum..." button that sets how many entries the list has, growing or shrinking it.
  - The names are what editor dropdowns show wherever that type is chosen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TYP-002 — Elements
- **What:** The list of damage element names.
- **Behavior:**
  - The names appear in element selectors.
  - How an element actually behaves is defined in skill and weapon data, not here.
  - Traits use elements for Element Rate and Attack Element.
  - **(screenshot)** The sample has 9 entries; entry 01 is Physical.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TYP-003 — Skill Types
- **What:** The names of the skill categories that skills are classified into.
- **Behavior:**
  - These names are offered when choosing a skill's Skill Type.
  - Traits use them for Add Skill Type and Seal Skill Type. System 2 uses them for [SV] Magic Skills.
  - **(screenshot)** The sample has 2 entries (Magic, Special).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TYP-004 — Weapon Types
- **What:** The names of the weapon categories.
- **Behavior:**
  - These names are offered when choosing a weapon's type.
  - Traits use them for Equip Weapon. System 2 uses them for the rows of [SV] Attack Motions.
  - **(screenshot)** The sample has 12 entries (Dagger through Spear).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TYP-005 — Armor Types
- **What:** The names of the armor categories.
- **Behavior:**
  - These names are offered when choosing an armor's type.
  - Traits use them for Equip Armor.
  - **(screenshot)** The sample has 6 entries, including general, magic, light and heavy armor plus small and large shields.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TYP-006 — Equipment Types (slots)
- **What:** The names of the equipment slots. Each armor is assigned to one of these slots.
- **Behavior:**
  - These names are offered when choosing an armor's Equipment Type.
  - Traits use them for Lock Equip and Seal Equip.
  - **(screenshot)** The sample has 5 entries: Weapon, Shield, Head, Body and Accessory.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

---

### TRM: Terms

### RM-TRM-001 — Terms (vocabulary) editing model
- **What:** A screen of text fields that rename the words the game uses for stats, commands and system messages.
- **Behavior:**
  - Each slot is a free-text field filled with a default word.
  - Very long text may be cut off in game.
  - The Database index describes Terms as the names of common commands and parameters.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-TRM-002 — Basic Statuses terms
- **What:** The names for Level, HP, MP, TP and EXP.
- **Behavior:**
  - Each stat has a full name and an "(abbr.)" short form. The short form is used in tight spaces such as the battle status window.
  - **(screenshot)** There are 10 slots: Level, Level (abbr.), HP, HP (abbr.), MP, MP (abbr.), TP, TP (abbr.), EXP and EXP (abbr.).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-TRM-003 — Parameters terms
- **What:** The display names of the battler parameters.
- **Behavior:**
  - **(screenshot)** There are 10 slots: Max HP, Max MP, Attack, Defense, M.Attack, M.Defense, Agility, Luck, Hit Rate and Evasion Rate.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-TRM-004 — Equipment Type terms
- **What:** The page text lists a group that names the equipment slots.
- **Behavior:**
  - The text says this group renames the equipment locations.
  - **(screenshot)** The Terms screenshot shows no such group. In MZ the slot names appear under Types → Equipment Types (RM-TYP-006).
  - Whether a separate Terms group exists in MZ is therefore uncertain (see Not verified).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html

### RM-TRM-005 — Commands terms
- **What:** The labels for commands and choices in menus, battle, shops and the title screen.
- **Behavior:**
  - **(screenshot)** There are 24 slots:
    - Fight, Escape, Attack, Guard
    - Item, Skill, Equip, Status
    - Formation, Options, Save, Game End
    - Weapon, Armor, Key Item, Equip (a second slot of that name, in the equipment/item area)
    - Optimize, Clear, Buy, Sell
    - New Game, Continue, To Title, Cancel
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-TRM-006 — Messages terms
- **What:** The text templates for system messages: options labels, save/load prompts, battle announcements, rewards and similar.
- **Behavior:**
  - Templates can contain numbered placeholders (%1, %2, ...), which are replaced with actual values when the message is shown.
  - **(screenshot)** The messages are shown as a Type / Text table.
  - **(screenshot)** Message slot names visible in the screenshot:
    - Options screen: Always Dash, Command Remember, Touch UI, BGM Volume, BGS Volume, ME Volume, SE Volume
    - Status: Possession, EXP Total, EXP Next
    - Save/load: Save Message, Load Message, File, Autosave
    - Battle: Party Name, Emerge, Preemptive, Surprise, Escape Start, Escape Failure, Victory, Defeat
    - Rewards: Obtain EXP, Obtain Gold, Obtain Item, Level Up, Obtain Skill
    - Actions: Use Item, Critical to Enemy
  - The table has a scrollbar and continues below; the remaining slots are not visible (see Not verified).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

---

### TRT: Traits

### RM-TRT-001 — Traits system (general)
- **What:** A list of property entries that can be attached to actors, classes, weapons, armors, enemies and states.
- **Behavior:**
  - There are 25 trait kinds, grouped into six tabs. **(screenshot)** The tabs are Rates, Param, Attack, Skill, Equip and Other.
  - The traits of a character's class, equipped weapons and armor, and current states are merged into that character's effective traits while each source applies.
  - **Editing:**
    - Double-click an empty row to add a trait: pick a kind in the dialog, then set its parameters.
    - Double-click an existing entry to edit it.
    - The right-click menu offers copy, delete and similar actions.
  - Some traits restrict the player permanently. For example, a weapon carrying Lock Equip cannot be taken off once equipped.
  - **(screenshot)** The dialog uses one radio button per trait kind, with a target dropdown and, where needed, a "*" percentage spinner. A new Element Rate shows 100 %.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-002 — Element Rate (Rates tab)
- **What:** Multiplies the damage this character takes from a given element.
- **Behavior:**
  - Parameters: an element and a rate from 0 to 1000%.
  - 100% means normal damage, above 100% means a weakness and below 100% means resistance.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-003 — Debuff Rate (Rates tab)
- **What:** Scales how likely this character is to receive a debuff on a given parameter.
- **Behavior:**
  - Parameters: a parameter and a rate from 0 to 1000%. 100% means unchanged.
  - It applies to skills and items that use the Add Debuff effect.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-004 — State Rate (Rates tab)
- **What:** Scales how likely this character is to receive a given state.
- **Behavior:**
  - Parameters: a state and a rate from 0 to 1000%. 100% means unchanged.
  - It applies to skills and items that use the Add State effect.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-005 — State Resist (Rates tab)
- **What:** Makes this character fully immune to one state.
- **Behavior:**
  - Parameter: a state (dropdown only, no rate).
  - Choosing Knockout means the character is not knocked out even when HP falls to 0.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-006 — Regular Parameters (Param tab)
- **What:** A percentage multiplier on a basic parameter.
- **Behavior:**
  - Parameters: a parameter (for example Max HP or Attack) and a rate from 0 to 1000%. 100% means unchanged.
  - The full list of selectable parameters is not given on this page. The Terms screenshot shows eight basic parameter names: Max HP, Max MP, Attack, Defense, M.Attack, M.Defense, Agility and Luck.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-TRT-007 — Ex-Parameters (Param tab)
- **What:** Adds a flat percentage to a rate-type "extra" parameter.
- **Behavior:**
  - Parameters: an ex-parameter (examples given: Hit Rate, Evasion Rate) and an amount from -100 to 100%.
  - The base value is 0%, and the trait adds to it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-008 — Sp-Parameters (Param tab)
- **What:** A multiplier on a "special" parameter.
- **Behavior:**
  - Parameters: an sp-parameter (examples given: Target Rate, Guard Rate) and a rate from 0 to 1000%.
  - The base value is 100%.
  - The Effects page names Pharmacology as a special-parameter value that scales item recovery, so Pharmacology belongs to this group.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-TRT-009 — Attack Element (Attack tab)
- **What:** Gives the character's normal attack an element.
- **Behavior:**
  - Parameter: an element.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-010 — Attack State (Attack tab)
- **What:** Gives the character's normal attack a chance to inflict a state.
- **Behavior:**
  - Parameters: a state and a success rate from 0 to 1000%. 100% means unchanged.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-011 — Attack Speed (Attack tab)
- **What:** Changes the character's agility on turns where it chooses a normal attack. The page does not say what that agility is used for, presumably action order.
- **Behavior:**
  - Parameter: a value from -999 to 999 that is added to or subtracted from agility.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-012 — Attack Times + (Attack tab)
- **What:** Increases how many times a normal attack hits its target.
- **Behavior:**
  - The base is 1 hit. The parameter is how many hits to add.
  - The allowed range is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-013 — Attack Skill (Attack tab)
- **What:** Replaces what the Attack command does with a chosen skill.
- **Behavior:**
  - Parameter: a skill.
  - When the character picks Attack, that skill runs instead. This allows, for example, weapons whose attack triggers a special skill.
  - The trait is new in MZ. The improvements page describes it as available in actor and weapon traits.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-TRT-014 — Add Skill Type (Skill tab)
- **What:** Gives the character the command for a skill type, so the player can select it. The page does not say whether this means battle, menu or both.
- **Behavior:**
  - Parameter: a skill type.
  - The command for that skill type becomes selectable.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-015 — Seal Skill Type (Skill tab)
- **What:** Blocks a whole skill type.
- **Behavior:**
  - Parameter: a skill type.
  - The player cannot select that skill type.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-016 — Add Skill (Skill tab)
- **What:** Grants a specific skill while the trait applies.
- **Behavior:**
  - Parameter: a skill.
  - The skill becomes usable.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-017 — Seal Skill (Skill tab)
- **What:** Blocks one specific skill.
- **Behavior:**
  - Parameter: a skill.
  - The skill cannot be used.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-018 — Equip Weapon (Equip tab)
- **What:** Allows a weapon type to be equipped.
- **Behavior:**
  - Parameter: a weapon type.
  - Weapons of that type become equippable.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-019 — Equip Armor (Equip tab)
- **What:** Allows an armor type to be equipped.
- **Behavior:**
  - Parameter: an armor type.
  - Armor of that type becomes equippable.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-020 — Lock Equip (Equip tab)
- **What:** Freezes an equipment slot so its contents cannot be changed.
- **Behavior:**
  - Parameter: an equipment type (slot).
  - The help suggests it for guest actors who join the party temporarily.
  - When the trait sits on an equipped item, that item becomes impossible to remove (see RM-TRT-001).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-021 — Seal Equip (Equip tab)
- **What:** Prevents anything from being equipped in a slot.
- **Behavior:**
  - Parameter: an equipment type (slot).
  - Typical uses: a two-handed weapon that seals the shield slot, or full-body clothing that seals the head slot.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-022 — Slot Type (Equip tab)
- **What:** Changes the character's slot layout.
- **Behavior:**
  - The only value is Dual Wield.
  - With Dual Wield the character can hold two weapons but loses the ability to equip a shield.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-023 — Action Times + (Other tab)
- **What:** A chance of getting one extra action per turn in battle.
- **Behavior:**
  - Parameter: a probability.
  - Each instance of the trait is rolled independently and can add one action.
  - Example: two 50% entries give a 25% chance of two extra actions and a 25% chance of none.
  - The probability range is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-024 — Special Flag (Other tab)
- **What:** Switches on one of several special battle behaviors.
- **Behavior:** Parameter: one flag.
  - **Auto Battle:** the character picks its own actions; the player gives it no commands.
  - **Guard:** damage taken is reduced by a fixed rate. The rate is not stated.
  - **Substitute:** the character takes hits for an ally whose HP is low. The threshold is not stated.
  - **Preserve TP:** TP carries over to the next battle. Normally TP resets each battle and starts at a random amount.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-TRT-025 — Collapse Effect (Other tab)
- **What:** Changes the visual effect when an enemy is defeated.
- **Behavior:**
  - It only works on enemies.
  - Parameter: a collapse effect type.
  - "Boss" is one option, confirmed by the System 1 sounds Boss Collapse 1 and Boss Collapse 2, which play for enemies whose collapse effect is Boss.
  - The other option names are not given.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-TRT-026 — Party Ability (Other tab)
- **What:** A party-wide perk.
- **Behavior:**
  - The perk applies to the whole party if even one member has the trait.
  - Parameter: one ability.
  - **Half Encounters:** random encounters on the map happen half as often.
  - **Encounter None:** there are no random encounters on the map.
  - **Cancel Surprise:** prevents surprise starts, where only enemies act on turn 1.
  - **Raise Preemptive:** makes preemptive starts, where only the party acts on turn 1, more likely. How much more likely is not stated.
  - **Gold Double:** gold from battle victories is doubled.
  - **Drop Item Double:** the chance of enemy item drops is doubled. It only matters for enemies that have drop items set.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

---

### EFF: Effects

### RM-EFF-001 — Effects system (general)
- **What:** A list of effect entries on a skill or item that are applied to the target when an actor or enemy uses it.
- **Behavior:**
  - There are 13 effect kinds, grouped into four tabs: Recover, State, Param and Other.
  - Multiple effects can be combined on one skill or item.
  - **Editing:**
    - Double-click an empty row to add an effect: pick a kind, then set its target and strength.
    - Double-click an existing entry to edit it.
    - The right-click menu offers copy, delete and similar actions.
  - **(screenshot)** The dialog uses one radio button per effect kind. Recover HP and Recover MP each show a "%" spinner and a "+" flat-value spinner. A new Recover HP shows 100 % + 0.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-002 — Recover HP (Recover tab)
- **What:** Adds HP to the target. Negative values remove HP.
- **Behavior:**
  - Amount = (a percentage of the target's Max HP, from -100% to 100%) + (a flat value, from -999999 to 999999).
  - Set either part to 0 to use only the other.
  - On items, the amount is further scaled by the user's Pharmacology special parameter.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-003 — Recover MP (Recover tab)
- **What:** Adds MP to the target. Negative values remove MP.
- **Behavior:**
  - Amount = (a percentage of the target's Max MP, from -100% to 100%) + (a flat value, from -9999 to 9999).
  - Set either part to 0 to use only the other.
  - On items, the amount is scaled by the user's Pharmacology.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-004 — Gain TP (Recover tab)
- **What:** Adds TP to the target.
- **Behavior:**
  - Parameter: a flat amount from 0 to 100.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-005 — Add State (State tab)
- **What:** Tries to inflict a state on the target.
- **Behavior:**
  - Parameters: a state and a success rate from 0 to 1000%.
  - Values above 100% make success more likely than the state's base effectiveness.
  - Target resistances are applied through the State Rate and State Resist traits.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-EFF-006 — Remove State (State tab)
- **What:** Tries to remove a state from the target.
- **Behavior:**
  - Parameters: a state and a success rate from 0 to 1000%.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-007 — Add Buff (Param tab)
- **What:** Raises one parameter's buff level by one step for a number of turns.
- **Behavior:**
  - Parameters: a target parameter and a duration from 1 to 1000 turns.
  - Each level changes the parameter by 25% of its base value.
  - To raise more than one level at once, add the effect more than once.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-008 — Add Debuff (Param tab)
- **What:** Lowers one parameter's buff level by one step for a number of turns.
- **Behavior:**
  - Parameters: a target parameter and a duration from 1 to 1000 turns.
  - Each level is 25% of the base value.
  - Add the effect more than once to lower more than one level.
  - Its success is affected by the target's Debuff Rate trait.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-EFF-009 — Remove Buff (Param tab)
- **What:** Clears a positive buff on one parameter.
- **Behavior:**
  - Parameter: a target parameter.
  - If that parameter is currently buffed, it returns to its base value.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-010 — Remove Debuff (Param tab)
- **What:** Clears a negative buff on one parameter.
- **Behavior:**
  - Parameter: a target parameter.
  - If that parameter is currently debuffed, it returns to its base value.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-011 — Special Effect: Escape (Other tab)
- **What:** Makes the target leave the battle.
- **Behavior:**
  - The only value is Escape.
  - An actor who leaves this way gets no EXP from the battle.
  - If every party member leaves through this effect, the battle counts as a loss.
  - This is a different mechanism from the party-level Escape command.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-012 — Grow (Other tab)
- **What:** Permanently increases a parameter on the target.
- **Behavior:**
  - Parameters: a target parameter and an amount from 1 to 1000 that is added.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-013 — Learn Skill (Other tab)
- **What:** Teaches the target actor a skill.
- **Behavior:**
  - Parameter: a skill.
  - It works only on actors and has no effect on enemies.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-EFF-014 — Common Event (Other tab)
- **What:** Runs a common event when the skill or item is used.
- **Behavior:**
  - Parameter: a common event.
  - One skill or item can have several Common Event entries.
  - In MZ every configured common event runs. The improvements page presents this as a change from earlier behavior.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

---

## 3. Not verified

Each point below was requested or is relevant, but the fetched official pages do not state it. Do not assume any of these without another official source.

**System 1 / System 2**
- Default values for System 1 and System 2 fields. Neither page gives defaults; screenshot values are only from a sample project.
- **Window opacity:** no such field appears in the System 1 or System 2 text or screenshots.
- **Test battle settings** in System: not on these pages.
- **Editor settings** in System: not on these pages.
- **Locale / language** setting: not on these pages.
- **Picture-related** advanced settings: not on these pages.
- **Advanced Settings:** the screenshot shows only Game ID, Screen Width/Height, UI Area Width/Height, Main/Number font filenames, Fallback Fonts and Font Size. No other advanced fields are documented.
- Allowed ranges for Screen Width/Height and UI Area Width/Height, and how the UI area relates to the screen (centering, scaling).
- Allowed range for Font Size, and which font file formats are accepted (the screenshot shows `.woff` names only).
- How the Game ID is generated, and its format and limits beyond "editable" and the 8-digit sample.
- **Title Command Window:** the list of background types and the range of the position offset.
- **Title Screen image picker:** what its two lists are, presumably a background layer and a frame layer; this is not stated.
- **Vehicle Images:** which image source or sheet they are picked from.
- **Menu Commands:** whether the list shown (Item, Skill, Equip, Status, Formation, Save) is complete, and whether Options or Game End can be toggled.
- **[SV] Attack Motions:** the full list of selectable motions (the screenshot shows Thrust, Swing and Missile) and the source of the weapon images.
- **Time Progression Battles:** how fast the gauge fills, how agility enters the formula, and how Active and Wait differ beyond "time passes / doesn't pass during command input".
- How much damage slip damage and floor damage deal: not on these pages.
- Whether "Show Player Followers" can be toggled during play.

**Types**
- Maximum number of entries in each type list, and the default count in a new project. The screenshot counts come from a sample project.

**Terms**
- The complete list of Messages slots. The screenshot list is scrolled, so slots after "Critical to Enemy" are not visible.
- Whether a separate "Equipment Type" group exists in the MZ Terms screen. The text lists one, the screenshot does not show one.
- Maximum text length per term.
- Which placeholder indices each message supports.

**Traits**
- The full list of parameters selectable for Regular Parameters, Debuff Rate and Grow/Buff. Only examples are named; the Terms screenshot suggests the eight basic parameters.
- The full list of Ex-Parameters. Only Hit Rate and Evasion Rate are named.
- The full list of Sp-Parameters. Only Target Rate and Guard Rate are named on the traits page, and Pharmacology is implied by the effects page.
- How multiple traits of the same kind combine (multiplied, added, highest wins) for Element Rate, State Rate, Debuff Rate, Param, Ex-Param, Sp-Param, Attack Speed and Attack Times. Only Action Times + has its stacking described.
- The allowed range for Attack Times + and the probability range for Action Times +.
- The Collapse Effect option names other than Boss.
- The damage reduction applied by the Guard flag, and the HP threshold that triggers Substitute.
- How much Raise Preemptive increases the chance, and what the base preemptive and surprise chances are.
- Whether Attack Skill can be set on data types other than actors and weapons. The traits tab layout suggests it, but the page doesn't say so.
- The random range for starting TP when Preserve TP is off.
- Whether the four-member battle limit can be changed.

**Effects**
- The maximum number of buff and debuff levels, and what happens when a buff and a debuff meet on the same parameter.
- The exact formula for how Pharmacology scales item recovery.
- How the Add State success rate combines with the target's State Rate trait, luck or other factors.
- What Remove Buff or Remove Debuff does when the parameter is not in that condition (presumably nothing; not stated).
- Whether Learn Skill does anything if the actor already knows the skill.
- When the Common Event effect runs (immediately, or after the action) and in what order when there are several.
- Whether Gain TP can be negative. The documented range is 0 to 100.
- Which parameters Grow can target.
