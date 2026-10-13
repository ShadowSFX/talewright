# 08 — Runtime Systems (RPG Maker MZ, official sources only)

Clean-room feature inventory for Talewright. Everything below is written in our own words and paraphrases official pages. No sentences or code were copied. Anything the official pages do not state is marked "not verified".

Retrieval date: 2026-10-13. Help pages were downloaded as raw HTML from the official help site and read in full. The help index is a RoboHelp shell whose table of contents is in `whxdata/toc*.new.js`, which was used to find page names.

## Pages covered

### Official MZ Help (https://rpgmakerofficial.com/product/MZ_help-en/)
| Page | Title | Status |
|---|---|---|
| https://rpgmakerofficial.com/product/MZ_help-en/ | Help index (RoboHelp shell; TOC read from whxdata/toc*.new.js) | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html | Main Features of RPG Maker MZ | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html | MV Data Conversion | Loaded (200), nothing in this slice |
| https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html | Time Progression Battles | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html | Improvements to Artistic Expressions in Games | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html | Game Control Improvements | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_02.html | Getting Started Making Your Game | Loaded (200), nothing in this slice |
| https://rpgmakerofficial.com/product/MZ_help-en/01_03.html | Basic Editor Controls | Loaded (200), nothing in this slice |
| https://rpgmakerofficial.com/product/MZ_help-en/01_04.html | Menu Bar Content | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_05.html | How to Use Aid Tools | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_06.html | Playing Your Game | Loaded (200); also read via WebFetch |
| https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html | Map Properties | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html | Actor Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html | Class Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html | Skill Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html | Item Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html | Weapon/Armor Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html | Enemy Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html | Troop Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html | State Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html | System 1 Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html | System 2 Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html | Type Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html | Terms Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html | How to Set Traits | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html | How to Set Effects | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html | Event Commands: Messages | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html | Event Commands: Game Progression | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html | Event Commands: Flow Control | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html | Event Commands: Party | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html | Event Commands: Actor | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_09.html | Event Commands: Timing | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html | Event Commands: Audio & Video | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html | Event Commands: Scene Control | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html | Event Commands: System Settings | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html | Event Commands: Map | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html | Event Commands: Battle | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html | Event Commands: Advanced | Loaded (200), nothing in this slice |
| https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html | Asset Standards | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html | Side-View Character Standards | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html | Output Formats | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html | Using Official Plugins | Loaded (200) |
| https://rpgmakerofficial.com/product/MZ_help-en/01_11.html | (guessed parent page) | **404, not a real page** |

### Official API documentation (https://developer.rpgmakerweb.com/rpg-maker-mz/)
The official API docs cover only the core layer: Bitmap, ColorFilter, Graphics, Input, JsExtensions, JsonEx, Point, Rectangle, ScreenSprite, Sprite, Stage, Tilemap, TilingSprite, TouchInput, Utils, Video, Weather, WebAudio, Window and WindowLayer. **They contain no pages for Scene_\*, Window_\* subclasses, Game_\* or the \*Manager classes.**

| Page | Status |
|---|---|
| https://developer.rpgmakerweb.com/rpg-maker-mz/ (index) | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/Input.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/TouchInput.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/WebAudio.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/Window.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/Graphics.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html | Loaded (200) |
| https://developer.rpgmakerweb.com/rpg-maker-mz/Video.html | Loaded (200) |
| .../Scene_Menu.html, Scene_Title.html, Window_Message.html, Window_Base.html, Game_Action.html, Game_BattlerBase.html, AudioManager.html, DataManager.html, StorageManager.html, BattleManager.html | **404, these classes are not in the official API docs** |

### Official sites (rpgmakerweb.com)
| Page | Status |
|---|---|
| https://www.rpgmakerweb.com/products/rpg-maker-mz | Loaded via WebFetch (200) |
| https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-1-music-battles-animations | Loaded (200) |
| https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-2-graphics-mapping-eventing | Loaded (200), nothing in this slice |
| https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command | Loaded (200) |
| https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price | Loaded (200) |
| https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look | Loaded (200) |
| https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects | Loaded (200); see the caveat below |

**Caveat on the "Primer" blog post.** It is published on the official rpgmakerweb.com blog as a tutorial. It was written by a community author (Trihan) and updated by Archeia. It is MZ-aware: it mentions the MZ-only "Attack Skill" trait. It gives engine numbers that the help never states. In this file, any fact that comes only from the Primer is labelled **(Primer)**. Where the Primer contradicts the help, both values are shown, and the help is treated as authoritative for editor ranges.

Community forum threads (forums.rpgmakerweb.com) were returned by searches but were **not** used, because they are not official documentation.

---

## MNU — Menus and screens

### RM-MNU-001 — Title screen
- **What:** The first screen shown when the game launches. It offers the commands to start, resume or configure the game.
- **Behaviour:**
  - Three commands: **New Game** starts from the beginning, **Continue** opens the save-file list to resume, and **Options** opens global settings such as BGM volume.
  - The background is built from two layered images: a main background from `img/titles1` and a frame/overlay from `img/titles2`. The documented size is 816x624.
  - The **Draw Game Title** option renders the game title text near the top of the screen. It can be turned off when the artwork already contains the title.
  - The designer can set the command window's background type and position. The blog calls this the "Title Screen Command Window Offset".
  - A Title BGM is configurable in System 1.
  - The window title bar also shows the game title.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price

### RM-MNU-002 — Main menu (field menu)
- **What:** The menu opened while walking on the map with the Menu button. It holds party-management commands and system commands.
- **Behaviour:**
  - It opens with the Menu button. On keyboard and mouse this is the same input as Cancel; on a gamepad Menu is a separate button (Y).
  - The command list sits in the upper-left area. Commands: **Items, Skills, Equip, Status, Formation, Options, Save, End Game**.
  - The menu shows the party's money using the currency unit name from System 1.
  - The game-wide timer pauses while the menu is open (see MNU-020).
  - Each command can be shown or hidden per project (see MNU-003).
  - The "Open Menu Screen" event command can force the menu open. It is not usable in battle events.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html

### RM-MNU-003 — Menu command visibility and access control
- **What:** Project and runtime switches that decide which menu commands exist and whether the menu, saving and formation are allowed.
- **Behaviour:**
  - System 2 → **Menu Commands** turns individual commands in the menu screen on or off.
  - System 2 → **Item Categories** turns individual item-screen categories on or off. The preview blog's example is removing the Armor category when a game has no armor.
  - Event commands control access at runtime. Each setting stays in force until the same command changes it again:
    - **Change Menu Access** enables or disables the player opening the menu.
    - **Change Save Access** enables or disables saving.
    - **Change Formation Access** enables or disables reordering the party.
    - **Enable/Disable Encounters** turns random battles on or off.
  - How a disabled command is drawn (greyed, hidden or rejected with the buzzer) is not verified for every case. The Buzzer SE is documented as the sound for picking an unusable menu command (see MNU-019).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-MNU-004 — Item screen
- **What:** Lists the party's inventory and lets the player use items.
- **Behaviour:**
  - Categories: Item, Weapon, Armor and Key Item. The help uses these names for the shop sell categories. Each category can be toggled in System 2.
  - Picking a usable item applies it. Some items first ask the player to choose a target actor.
  - Only items whose Occasion is **Always** or **Menu Screen** can be used here. **Battle Screen** and **Never** items cannot.
  - Key Items appear in their own section, separate from regular items.
  - **Hidden Item A/B** types never appear on this screen. They show up only in the event-driven Select Item window.
  - **Show Key Item Quantities** (System 1) controls whether counts are drawn for key items.
  - A Consumable item drops by 1 in quantity each time it is used.
  - Using an item from the menu plays the Use Item SE.
  - The selected entry's description is shown.
  - Each item's stock is clamped to 0–99; 99 is the stated maximum a player can hold.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

### RM-MNU-005 — Skill screen
- **What:** Lets the player browse an actor's skills and use them outside battle.
- **Behaviour:**
  - Skills are grouped by skill type. Magic and TP Skills are the default types and are renamable.
  - An actor can use a skill type only if it was granted by an "Add Skill Type" trait and is not sealed.
  - Only skills whose Occasion is Always or Menu Screen are usable here.
  - Usability also requires:
    - enough MP and TP to pay the cost;
    - the required weapon type equipped, if one is set (either of two types is enough);
    - the skill not being sealed.
  - Some skills prompt for a target actor.
  - Each skill's description is shown when it is highlighted.
  - Using a skill from the menu plays the Use Skill SE.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-MNU-006 — Equip screen
- **What:** Lets the player change an actor's equipment.
- **Behaviour:**
  - Flow: pick an actor, then a slot, then the new item. An "empty" entry unequips the slot.
  - Default slot names are Weapon, Shield, Head, Body and Accessory. The names are renamable under Types/Terms.
  - Equippability comes from Equip Weapon / Equip Armor traits.
  - A **Lock Equip** trait makes a slot unchangeable.
  - A **Seal Equip** trait blocks a slot entirely. Examples: a two-handed weapon that seals the shield slot, or full-body clothing that seals the head slot.
  - **Dual Wield** replaces the shield slot with a second weapon slot.
  - Equipment adds flat parameter changes: −5000..5000 for Max HP/MP and −500..500 for the other parameters.
  - Changing equipment plays the Equip SE.
  - A before/after parameter comparison display is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-MNU-007 — Status screen
- **What:** A read-only detail view of one actor.
- **Behaviour:**
  - The player chooses which actor to view.
  - The actor's nickname appears in the upper right.
  - The Profile text appears along the bottom. Profile text accepts control characters.
  - The class name is shown in the menu and near the top of the status list.
  - The face image is the menu-screen portrait.
  - The exact field layout (parameters, EXP to next level, equipment list) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MNU-008 — Formation
- **What:** Reorders the party.
- **Behaviour:**
  - The player swaps party members' positions.
  - Only the first four members take part in battle, so formation decides who fights.
  - Followers on the map show at most the first four actors.
  - Formation can be disabled at runtime with Change Formation Access.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html

### RM-MNU-009 — Options screen
- **What:** A player-facing settings screen, reachable from both the title screen and the main menu.
- **Behaviour:**
  - It adjusts game-wide settings. BGM volume is the documented example.
  - The full option list is not verified. The help does not enumerate items such as other volume channels, Always Dash, Command Remember or a Touch UI toggle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-MNU-010 — Save screen and Load (Continue) screen
- **What:** The slot-selection screens for writing and reading save data.
- **Behaviour:**
  - The Save command and the "Open Save Screen" event command both open a list of 20 save slots.
  - Continue on the title screen opens the save-file list for loading.
  - The top slot doubles as the autosave slot (see SAV-002).
  - Saving and loading play the Save and Load SEs.
  - Terms → Messages includes Save Message and Load Message strings, which accept control characters.
  - The official AltSaveScreen plugin offers another layout that can show images such as party faces.
  - The default per-slot contents (playtime, party images and so on) are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-MNU-011 — Game End
- **What:** A command in the main menu for leaving the current session.
- **Behaviour:**
  - It offers two choices: **To Title**, which returns to the title screen, and **Cancel**, which goes back.
  - The "Return to Title Screen" event command does the same thing on demand.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html

### RM-MNU-012 — Shop screen
- **What:** A buy/sell screen opened by the Shop Processing event command.
- **Behaviour:**
  - **Merchandise list.** Each entry uses either the database price or a price set in the command.
  - **Purchase Only** removes selling.
  - **Buy flow:**
    - The player picks an item, enters a quantity and confirms.
    - A side window shows how many the party already owns.
    - For weapons and armor, party members who cannot equip the item are shown greyed.
    - With five or more members, the extra members are paged in with the Dash button or by clicking/tapping the side window.
  - **Sell flow:**
    - The player picks a category (Item, Weapon, Armor or Key Item), then an owned item, then a quantity, and confirms.
    - An item sells for half its price. A price of 0 makes it unsellable.
  - **Quantity entry:** Right is +1, Left is −1, Up is +10, Down is −10. On-screen buttons do the same for touch, plus an OK button.
  - A Shop SE plays on each buy or sell.
  - The command cannot be used in battle events.
  - Interaction with the 99-item cap is not verified beyond the general 0–99 clamp.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MNU-013 — Name input screen
- **What:** A screen where the player types a new name for an actor. It is opened by Name Input Processing.
- **Behaviour:**
  - The command sets the target actor and a maximum length of 1–16 characters.
  - The player moves the cursor over a character grid with the D-pad.
  - The confirm button adds the highlighted character, and a delete button removes the last one.
  - The result replaces the actor's name.
  - It cannot be used in battle events.
  - The character table layout and the exact key for "delete" are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html

### RM-MNU-014 — Game Over screen
- **What:** The terminal screen shown on party defeat or when the Game Over event command forces it.
- **Behaviour:**
  - It shows the `img/system/GameOver.png` image and plays the Game Over ME from System 1.
  - It is reached when every party member is knocked out, unless the battle was started with Can Lose (see BTL-019).
  - The Game Over event command forces this screen.
  - What happens after the screen (for example, returning to the title) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-MNU-015 — Terms (customizable UI vocabulary)
- **What:** A database page that renames everything the UI prints.
- **Behaviour:**
  - **Basic statuses:** Level, HP, MP, TP and EXP, each with a full name and an abbreviation used in compact places such as the battle status window.
  - **Parameter names.**
  - **Equipment type names.**
  - **Command names:** labels for menu and other commands and choices.
  - **Message templates:** in-game message texts in which `%1`, `%2` and so on are filled in at runtime. The save and load messages and the battle messages from "Party Name" onward accept control characters.
  - Over-long names may be cut off.
  - The complete list of command and message keys is not verified, because the help shows it only as a screenshot.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MNU-016 — Window look, resolution and fonts
- **What:** Global presentation settings shared by every menu window.
- **Behaviour:**
  - **Window Color:** an RGB tone of −255..255 per channel, set in System 1. The Change Window Color event command changes it at runtime.
  - **Window skin:** windows draw from `img/system/Window.png`. Text colour index `n` used by `\C[n]` also comes from Window.png.
  - **System 2 Advanced settings:**
    - Screen Width and Height.
    - UI Area Width and Height.
    - Main Font file, Number Font file and Fallback Font, the last used for characters missing from the main font.
    - Font Size.
  - The preview blog says resolution can now be set without plugins.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price

### RM-MNU-017 — Touch- and mouse-optimized menu UI
- **What:** MZ's default menus are designed to work with mouse and touch as well as keys or a gamepad.
- **Behaviour:**
  - Menus are laid out for both PC and smartphone.
  - Windows can be scrolled by swiping. The mouse wheel also scrolls.
  - Battle targets can be picked by touch.
  - On-screen buttons come from `img/system/ButtonSet.png`. They cover opening the menu, cancelling or going back, and changing shop quantities.
  - The official blog says every in-game menu was designed for mouse and touch without losing controller support.
  - The exact button placement per scene is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-MNU-018 — Alternative official layouts (bundled plugins)
- **What:** Official plugins that swap the layout of the default menu and save screens.
- **Behaviour:**
  - **AltMenuScreen** lays the party out horizontally in the menu.
  - **AltSaveScreen** gives the save and load screens a different layout that can show images such as party faces.
  - Both are optional and not part of the default behaviour.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-MNU-019 — UI sound-effect hooks
- **What:** The system SEs tied to menu interaction.
- **Behaviour:**
  - **Cursor** plays when the cursor moves.
  - **OK** plays when a command is confirmed.
  - **Cancel** plays when backing out.
  - **Buzzer** plays when an unusable command is chosen.
  - **Equip** plays when equipment changes.
  - **Save** and **Load** play when saving and loading.
  - **Shop** plays when buying or selling.
  - **Use Item** and **Use Skill** play when used from the menu.
  - Each SE is configured per project in System 1.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-MNU-020 — Timer display and menu pause
- **What:** The countdown timer overlay and how it behaves around menus.
- **Behaviour:**
  - Control Timer → Start shows the remaining time in the top-right corner. The range is 0:00 to 99:59.
  - The countdown pauses while the menu is open.
  - Stop halts the timer.
  - Events can branch on the remaining time.
  - Behaviour during battle is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html

---

## BTL — Battle systems and flow

### RM-BTL-001 — Battle system modes
- **What:** A project-wide choice among three battle systems, plus a separate camera choice.
- **Behaviour:**
  - **Turn-Based:** every battler's action is resolved within one shared turn. The help describes it as all battlers acting at one time.
  - **Time Progression (Active):** each battler acts when its own gauge fills, and the order is driven by agility. Time keeps running while the player is entering commands.
  - **Time Progression (Wait):** the same as Active, except time stops while the player is entering commands.
  - The mode is set in System 1 → Battle System.
  - The official product page sums up TPB as each character acting at their own speed based on stats and conditions.
  - Front-view or side-view is chosen separately (see BTL-021).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-1-music-battles-animations ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-BTL-002 — Starting a battle (encounters and event battles)
- **What:** How battles begin.
- **Behaviour:**
  - **Random encounters on a map:**
    - A map sets the average number of steps between encounters (1–999; one step is one tile).
    - It also lists candidate troops, each with a weight of 0–100. A troop's chance is its weight divided by the total weight; the help's example is 9/7/4, giving 45%/35%/20%.
    - Each troop can be limited to the whole map or to up to 3 region IDs. Maps have regions 1–255.
  - **Battle Processing event command:**
    - Picks the troop directly, through a variable, or by using the map's encounter table.
    - Can Escape enables the Escape command and adds "won" and "escaped" branches.
    - Can Lose prevents a game over and adds "won" and "lost" branches.
    - It is not usable inside battle events.
  - **Encounter modifiers:**
    - The Half Encounters party ability halves the encounter rate. The Primer says each step counts 0.5 instead of 1.
    - The Encounter None party ability stops random encounters.
    - The Enable/Disable Encounters event command toggles them.
  - On encounter, the Battle Start SE plays and the Battle BGM starts.
  - **Battleback:** a map can set two layered battleback images (floor and wall). If it does not, one is picked from the field terrain, or a processed image of the map is used on other maps.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-003 — Battle participants
- **What:** Who takes part in a battle.
- **Behaviour:**
  - The party can be any size, but only the first 4 members fight.
  - A troop holds up to 8 enemies, and the same enemy may appear more than once.
  - The order in which enemies were added to the troop is the order of the enemy selection list in battle.
  - Enemies flagged **Appear Mid-Battle** are absent until the Enemy Appearance command runs.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html

### RM-BTL-004 — Party command window
- **What:** The per-turn top-level choice in battle.
- **Behaviour:**
  - It offers **Fight**, which goes on to the actor commands, and **Escape**, which tries to run.
  - Escape is available only when the battle allows it (Can Escape).
  - On a failed escape, only the enemies act for the rest of that turn.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html

### RM-BTL-005 — Actor command window
- **What:** The per-actor choice of action.
- **Behaviour:**
  - Base commands:
    - **Attack** uses the equipped weapon. An "Attack Skill" trait can replace the skill that Attack runs.
    - **Guard** reduces incoming damage.
    - **Item** uses a carried item.
  - The actor also gets one command per skill type it has been granted, unless that type is sealed.
  - Available commands depend on the actor's traits.
  - Actors with the **Auto Battle** flag choose their actions automatically. The Primer says they pick the action and target with the highest estimated effect.
  - In TPB, when more than one actor is ready, the player can switch which actor is being commanded with the cancel button. This comes from the TPB preview blog.
  - Target selection supports touch (see MNU-017).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-006 — Action order (speed)
- **What:** How the order of actions is decided.
- **Behaviour:**
  - Order depends on agility.
  - Every skill and item has a **Speed** value (−2000..2000) that is added to the user's agility for that action. Designers use it to make fast-but-weak or slow-but-strong actions.
  - The **Attack Speed** trait adds a value to agility only for normal attacks. The help gives −999..999; the Primer gives −1000..1000.
  - In TPB modes, agility also sets the gauge fill rate (see BTL-007).
  - Whether turn-based ordering adds a random factor is not verified, and so is the exact speed formula.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-007 — Time Progression Battle (TPB) gauge mechanics
- **What:** The gauge model behind both TPB modes.
- **Behaviour:**
  - Each battler has a gauge that fills over time; the help page calls it a "TP gauge". When it reaches 100% the battler acts, or the player picks its command. After acting the gauge empties and starts filling again.
  - **Fill speed (preview #5 blog):**
    - Each battler's "TPB Speed" comes from the **square root of its agility**.
    - The fastest battler in the fight fills in **4 seconds**.
    - Everyone else fills at a rate relative to that fastest TPB Speed.
  - **Starting fill:**
    - In a normal start, each battler begins with a random 0–50% of its gauge.
    - In a surprise or preemptive start, the side with the advantage begins at 100%.
  - **Casting time:** skills and items can have a delay between selection and execution. Its length combines the skill's or item's casting value with the battler's TPB Speed. Which editor field supplies the casting value is not verified.
  - **Active vs Wait:** they differ only in whether time passes during command input (BTL-001).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look

### RM-BTL-008 — Turn counting under TPB
- **What:** What "turn" means when there are no global rounds.
- **Behaviour:**
  - **Individual turn count:** how many turns this battler has taken, so a battler starting a new turn after 3 is on turn 4. It drives most per-battler logic, such as state durations and enemy action-pattern turn conditions.
  - **Battle turn count:** equal to the individual count of the enemy that has taken the most turns. It drives battle-event turn conditions.
- **Source:** https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look

### RM-BTL-009 — Surprise and preemptive starts
- **What:** Opening-turn advantage for one side.
- **Behaviour:**
  - **Surprise:** only the enemy troop can act on the first turn.
  - **Preemptive:** only the party can act on the first turn.
  - **Cancel Surprise** (party ability) prevents surprise starts.
  - **Raise Preemptive** (party ability) makes preemptive starts more likely.
  - Under TPB, the side with the advantage starts with full gauges.
  - **Odds (Primer only):**
    - The odds depend on the party's average agility versus the troop's.
    - When the party's average is at least the troop's, preemptive is 5% and surprise 3%.
    - Otherwise the figures are 3% and 5%.
    - The Primer says Raise Preemptive adds to the preemptive chance; its wording suggests a 2% increase.
  - The exact formula is not verified by the help.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-010 — Escape rules
- **What:** How the party or individual battlers leave a battle.
- **Behaviour:**
  - **Party command Escape:**
    - Only available when Battle Processing allows escape.
    - If it fails, only the enemies act for the rest of the turn.
    - Success leads to the event's "escaped" branch.
    - An escape count is recorded in the game data that the Control Variables "Other" operand exposes.
  - **Escape special effect (skill or item):**
    - The target flees the battle. This is separate from the party command.
    - An actor who flees this way gets no EXP.
    - If every party member flees through this effect, the battle counts as **lost**.
    - The Primer says it always succeeds.
  - **Abort Battle (event):** see BTL-020.
  - The Escape SE plays whenever the party or an enemy escapes.
  - **Escape success ratio formula: not verified.** No official page states it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-011 — Enemy action selection (AI)
- **What:** The documented rule for choosing an enemy's action.
- **Behaviour:**
  - **Pattern conditions.** Each action pattern can have one of these:
    - **Turn A + B·X:** for example, A=2 and B=3 fires on turns 2, 5, 8, …
    - **HP %** within a range.
    - **MP %** within a range.
    - **State** present.
    - **Party Level:** the highest party level is at least the given value.
    - **Switch** ON.
  - **Selection steps:**
    1. Keep only the patterns whose conditions hold. If none hold, the enemy does nothing.
    2. Find the highest rating. Keep the patterns rated within 2 of it.
    3. Weight them: the top rating gets weight 1, top−1 gets 2/3 and top−2 gets 1/3. Equal ratings share equally.
    4. Pick at random by weight. The help's examples: ratings 6/5/4/3 give 50%/33.3%/16.6%/0, and ratings 5/3/3 give 60/20/20.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html

### RM-BTL-012 — Battle events (troop events)
- **What:** Scripted event pages that run during a battle.
- **Behaviour:**
  - **Page conditions** can be combined:
    - Turn End.
    - Turn number (start plus interval).
    - Enemy HP at or below a %.
    - Actor HP at or below a %.
    - Switch ON.
  - A page with **no** condition never runs. This is unlike map events.
  - When several pages qualify, the lowest-numbered page wins.
  - **Span:**
    - **Battle:** runs once per battle.
    - **Turn:** checked each turn.
    - **Moment:** repeats while the condition holds, which can stall the battle unless switches gate it.
  - **Battle-only commands:** Change Enemy HP/MP/TP/State, Enemy Recover All, Enemy Appearance, Enemy Transform (HP and MP carry over), Show Battle Animation, Force Action (targets: last target, random, or index 1–8) and Abort Battle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html

### RM-BTL-013 — Targeting and scope
- **What:** How an action's targets are defined and chosen.
- **Behaviour:**
  - **Scope** on a skill or item has three parts:
    - **Faction:** Enemy, Ally, Enemy & Ally, User, or Unspecified.
    - **Number:** one, all, or random.
    - **Status filter:** Alive, Knocked Out, or No Condition. Allies only.
  - The player picks the target from a list or by touch.
  - **Target Rate** (an Sp-parameter) weights how likely a battler is to be chosen as an enemy's target. The Primer describes a weighted random pick: roll within the sum of the target rates, then walk the members.
  - **Confusion-style restrictions** (see BTL-016) force the target choice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-014 — Multiple actions and multi-hit
- **What:** Ways a battler gets more actions or more hits.
- **Behaviour:**
  - **Action Times +** gives a chance to gain one extra action. Each copy of the trait is rolled separately; the help's example is two 50% traits giving a 25% chance of +2 and a 25% chance of +0.
  - **Attack Times +** adds hits to normal attacks. The default is 1 hit.
  - **Repeat** (1–9) on a skill or item applies its effect that many times per use.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-BTL-015 — Counterattack, magic reflection, substitute
- **What:** Reactive mechanics and which hit types trigger them.
- **Behaviour:**
  - **Physical Attack** hit type allows Counterattack and Substitute.
  - **Magic Attack** allows Magic Reflection and Substitute.
  - **Certain Hit** disables all three.
  - **Counter Attack** and **Magic Reflection** are Ex-parameter chances.
  - The **Substitute** flag makes the holder take hits aimed at allies whose HP is low. The Primer puts the threshold at 25% HP or less.
  - **Primer only:**
    - Counterattack needs the defender to be able to move.
    - Reflection does not tell harmful and helpful spells apart.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-016 — States during battle
- **What:** Runtime lifecycle of states.
- **Behaviour:**
  - **Restriction** is one of: None, Attack an Enemy, Attack Anyone, Attack an Ally, or Cannot Move. When several apply, the one lowest in that list wins.
  - **Icon priority** is 0–100. On a tie, the lower ID shows.
  - **Removal options:**
    - At battle end.
    - When a state with a different restriction is added.
    - Automatically after a number of turns between a designer-set lower and upper bound (0–9999), counted at **Action End** or **Turn End**. How the value is picked within the range is not verified.
    - On damage, with a 0–100% chance.
    - After walking 0–9999 steps on the map.
  - **Messages** cover four moments: actor gains the state, enemy gains it, it persists, and it is removed. `%1` is the target's name.
  - **Side view:** each state can set an SV motion and an overlay.
  - **State #1 (Knockout)** is applied automatically at 0 HP. If the whole party is knocked out, the game is over.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html

### RM-BTL-017 — Victory and rewards
- **What:** What happens when every enemy is defeated.
- **Behaviour:**
  - Each enemy defines EXP (1–9,999,999) and Gold (0–9,999,999), which the party receives on victory.
  - **Drop items:** each enemy lists drops with a 1/N chance, where N is 1–1000.
  - **Gold Double** doubles the gold. **Drop Item Double** doubles the drop rate; it only matters when the enemy has a drop.
  - **EXP for Reserve Members** (System 1) also gives EXP to members who did not fight.
  - The Experience Sp-parameter scales the EXP gained (Primer).
  - Actors who left through the Escape effect get no EXP.
  - The Victory ME plays. It can be changed with Change Victory ME.
  - The layout of the result display is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-018 — Level up
- **What:** Turning accumulated EXP into levels.
- **Behaviour:**
  - When an actor reaches the EXP threshold for the next level, its level rises by 1. Parameters follow the class curves, and skills learned at that level are gained.
  - The level is capped at the actor's Max Level (1–99).
  - **EXP thresholds** come from the class EXP curve, which has four inputs: Base, Extra, Acceleration A and Acceleration B (see FRM-020).
  - The Change EXP and Change Level event commands can optionally show a level-up message.
  - The exact battle level-up message text is in Terms and not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html

### RM-BTL-019 — Defeat
- **What:** Losing a battle.
- **Behaviour:**
  - The party loses when every member's HP is 0, meaning every member is Knocked Out. Normally this leads to Game Over.
  - With Can Lose, there is no game over and the event takes its "lost" branch.
  - If the entire party leaves through the Escape special effect, that also counts as a loss.
  - The Defeat ME plays. It can be changed with Change Defeat ME.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html

### RM-BTL-020 — Abort Battle
- **What:** An event command that ends a battle immediately.
- **Behaviour:**
  - It returns to the map. It is only usable inside battle events.
  - If the battle came from Battle Processing with Can Escape on, the "escaped" branch runs, but the escape counter does **not** go up.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html

### RM-BTL-021 — Front view vs side view
- **What:** The battle camera choice.
- **Behaviour:**
  - **Front-view** shows enemies from the front; the help mentions only enemies for this mode. Front-view enemy images live in `img/enemies`.
  - **Side-view** shows both enemies and actors from the side:
    - Actors use SV battler sheets from `img/sv_actors`.
    - Enemies use `img/sv_enemies`.
    - Actor shadows come from Shadow2.png, state overlays from States.png, and normal-attack weapon images from Weapons1–3.png.
  - **Side-view settings:**
    - System 2 → [SV] Attack Motions sets which motion plays for normal attacks with each weapon type.
    - System 2 → [SV] Magic Skills lists the skill types that use a casting motion.
    - Each state can set an SV motion and overlay.
  - **Battleback:** 1000x740, made of two layers (floor from battlebacks1, wall from battlebacks2). Front view crops the top of the image and side view crops the bottom.
  - The mode is set in System 1 → Battle Screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-1-music-battles-animations

### RM-BTL-022 — Side-view battler sheet and motion playback
- **What:** The format of side-view actor sprites and how they animate.
- **Behaviour:**
  - One image per actor, laid out as a 9x6 grid of 54 cells. The standard asset is 576x384.
  - Each run of 3 consecutive cells is one motion.
  - **Looping idle motions** play in the order 1-2-3-2.
  - **One-shot motions**, such as taking damage, play 1-2-3.
  - The full list of motion names and their positions on the grid is not verified in text, because it appears only in an image.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html

### RM-BTL-023 — Battle messages (log)
- **What:** Text shown during battle.
- **Behaviour:**
  - Each skill has a usage message of up to 2 lines. `%1` is replaced by the user's name and `%2` by the skill name. Preset buttons insert "casts", "does" or "used" phrasing.
  - States have add, persist and remove messages.
  - The battle messages in Terms (from "Party Name" onward) are templates with `%n` placeholders, and they accept control characters, including the wait and flow codes.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-BTL-024 — Battle animations and collapse effects
- **What:** Visual feedback for actions and deaths.
- **Behaviour:**
  - Each skill and item sets an animation that plays on use in battle. Each weapon sets the animation its attacks play.
  - Animations are particle effects authored in Effekseer (`.efkefc`). Angle, size and speed can be adjusted, and sound effects and screen or target flashes can be layered on.
  - **Collapse Effect** trait (enemies only) replaces the default death effect:
    - **Boss** collapse uses the Boss Collapse 1 and 2 SEs. The Primer describes a purple-tinted split and sinking effect.
    - **Instant** removes the sprite at once (Primer).
    - **No Disappear** leaves the sprite in place (Primer).
  - A normal enemy death plays the Enemy Collapse SE.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-1-music-battles-animations ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-BTL-025 — Battle sound-effect hooks
- **What:** The system SEs tied to battle events.
- **Behaviour:**
  - Slots (names as in the editor) and what triggers them: **Battle Start** (a fight begins), **Escape** (a side
    flees), **Enemy Attack** (an enemy uses its basic attack), **Enemy Damage** / **Ally Damage** (a foe / a party
    member takes damage), **Enemy Collapse** / **Ally Death** (a foe dies / a party member falls), **Boss Collapse
    1/2** (the special boss death effect), **Recovery** (anyone is healed), **Miss**, **Evasion** and **Magic
    Evasion** (physical attacks that fail or are dodged, magic that is dodged), **Magic Reflection** (a spell bounces back).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-BTL-026 — Slip damage and HP floor
- **What:** Damage from states between actions, and its lower limit.
- **Behaviour:**
  - With **Death by Slip Damage** on, damage from states such as Poison can reduce HP to 0. With it off, HP stops at 1.
  - HP regeneration (the HRG Ex-parameter, which can be negative) applies at turn end in battle. On the map it applies every 20 steps (Primer).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

---

## FRM — Formulas and parameters

### RM-FRM-001 — Damage formula expression
- **What:** Each skill or item has a designer-written expression that sets the base effect amount.
- **Behaviour:**
  - The expression can refer to the attacker as `a` and the target as `b`, through these fields:
    - `.atk` (Attack), `.def` (Defense), `.mat` (Magic Attack), `.mdf` (Magic Defense), `.agi` (Agility), `.luk` (Luck);
    - `.mhp` and `.mmp` (max HP/MP), `.hp`, `.mp` and `.tp` (current values), `.level`.
  - `v[n]` reads game variable n.
  - The four arithmetic operators are supported. The help's example is "attacker attack ×4 minus target defense ×2".
  - Element and guard modifiers are applied **outside** the expression, so designers should not include them.
  - The MaterialBase official plugin notes that its control characters can also be used in skill and item formulas.
  - Any grammar beyond the documented fields, `v[n]` and the four operators is not verified.
  - Behaviour when the result is negative is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_05.html

### RM-FRM-002 — Damage types
- **What:** What the computed amount does.
- **Behaviour:**
  - The help says a skill or item picks one of six damage types acting on HP or MP, and describes three
    behaviours: **Damage** lowers the value, **Recover** raises it, **Drain** moves the amount from the target to
    the user.
  - The exact six names and whether a "None" choice exists are not verified (see RM-SKL-017).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-FRM-003 — Variance
- **What:** Random spread applied to the computed amount.
- **Behaviour:**
  - The setting is 0–100%. The final amount is randomized within ±variance% of the computed value; for example, 100 with 20% variance lands between 80 and 120.
  - The distribution shape (uniform or otherwise) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-FRM-004 — Hit types and hit determination
- **What:** How a skill or item decides whether it connects.
- **Behaviour:**
  - **Certain Hit:** always connects. Counter, reflection and substitute cannot trigger.
  - **Physical Attack:** success uses the user's accuracy (Hit Rate) and the target's Evasion. Counter and substitute can trigger.
  - **Magic Attack:** success uses the target's Magic Evasion. Reflection and substitute can trigger.
  - Per the Primer:
    - Hit Rate is consulted only for Physical Attack.
    - Evasion applies only to Physical Attack.
    - Magic Evasion applies only to Magic Attack.
  - A miss plays the Miss SE. An evasion plays the Evasion or Magic Evasion SE.
  - The exact combination formula (for example, whether hit is multiplied by success, then evasion rolled separately) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-005 — Success rate
- **What:** A per-skill or per-item base chance.
- **Behaviour:**
  - **Success** is 0–100%. The real chance also depends on how susceptible the target is.
  - For state and debuff effects, the target's State Rate and Debuff Rate traits scale the chance (FRM-015).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-FRM-006 — Critical hits
- **What:** Chance-based critical damage.
- **Behaviour:**
  - Only skills and items with **Critical Hits = Yes** can crit.
  - The chance depends on the user's Critical Rate and the target's Critical Evasion. The Primer gives it as the user's crit rate × (1 − the target's crit evasion), so negative critical evasion raises the chance.
  - **Critical damage multiplier: not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-007 — Elements and element rate
- **What:** Elemental affinity scaling.
- **Behaviour:**
  - The element list is defined in Types.
  - A skill or item carries one element. A normal attack's element comes from **Attack Element** traits.
  - The **Element Rate** trait (0–1000%) scales damage taken from that element. Above 100% means a weakness.
  - **Primer:**
    - Multiple Element Rate traits for the same element multiply together; for example, 200% and 95% give 190%.
    - A rate cannot go below 0%, so the default engine cannot make absorption.
  - **How several attack elements combine** (for example, a normal attack with two attack elements) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_13.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-008 — Guard
- **What:** Damage reduction from the Guard command or the Guard flag.
- **Behaviour:**
  - The Guard command, or the **Guard** special flag (which is always on), cuts damage taken by a fixed rate.
  - Guard is applied outside the damage formula.
  - **Primer:**
    - The default reduction is half.
    - The **Guard Effect** (GRD) Sp-parameter makes the reduction stronger above 100% and weaker below.
  - The exact formula is not verified by the help.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-009 — Physical and magical damage rate
- **What:** Sp-parameters that scale incoming damage by hit type.
- **Behaviour (Primer):**
  - **Physical Damage** scales damage from Physical Attack actions.
  - **Magic Damage** scales damage from Magic Attack actions.
  - **Floor Damage** scales damage from damage tiles. The Primer gives a base of 10.
  - The order in which these multiply with element rate, guard, critical and variance is not verified.
- **Source:** https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-FRM-010 — Recovery effectiveness and Pharmacology
- **What:** Modifiers on healing.
- **Behaviour:**
  - **Recover HP/MP effects:**
    - An HP effect is a % of the target's max (−100..100%) plus a flat value (help: −999,999..999,999 for HP; Primer: −99,999..99,999).
    - An MP effect works the same way with a flat value of −9,999..9,999.
  - On **items**, the amount is scaled by the user's **Pharmacology** Sp-parameter.
  - **Recovery Effect** (REC) scales healing received (Primer).
  - **Primer:**
    - Pharmacology stacks with Recovery Effect and is applied after it.
    - When an item is used from the menu, the "user" is the party member with the highest Pharmacology who can move.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-011 — Regular parameters and their limits
- **What:** The eight base parameters.
- **Behaviour:**
  - The parameters are Max HP, Max MP, Attack, Defense, M.Attack, M.Defense, Agility and Luck.
  - **Actor values** come from class curves per level 1–99: Max HP 1–9999, Max MP 0–9999, others 0–999. Quick presets A–E and a curve generator (early or late growth) are available.
  - **Enemy values:** Max HP 1–999,999, Max MP 0–9,999, others 0–999.
  - **Equipment:** adds flat amounts (ranges in MNU-006).
  - **Parameter-rate trait:** 0–1000%. The Primer says these traits multiply together.
  - **Grow effect:** permanently adds 1–1000.
  - **Change Parameter command:** adds or removes 1–9999.
  - The final clamping limits of the runtime value are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-012 — Ex-parameters
- **What:** Additive percentage stats that start at 0%.
- **Behaviour:**
  - The trait adds a percentage: help range −100..100%; Primer range −1000..1000%. The Primer says these traits add together.
  - **Members (names per Primer; the help names only Hit Rate and Evasion Rate as examples):**
    - **Hit Rate:** physical only.
    - **Evasion Rate:** physical only.
    - **Critical Rate.**
    - **Critical Evasion.**
    - **Magic Evasion:** magic only.
    - **Magic Reflection:** reflects any magic, including healing.
    - **Counter Attack:** responds to physical attacks with a normal attack and needs the defender to be able to move.
    - **HP Regeneration, MP Regeneration, TP Regeneration:** % of max applied at each turn end in battle and every 20 steps on the map.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-013 — Sp-parameters
- **What:** Multiplicative rate stats that start at 100%.
- **Behaviour:**
  - The trait sets a rate of 0–1000%. The Primer says these traits multiply together.
  - **Members (names per Primer; the help names only Target Rate and Guard Rate as examples):**
    - **Target Rate:** the weight for being targeted.
    - **Guard Effect:** strength of guarding.
    - **Recovery Effect:** healing received.
    - **Pharmacology:** item healing; menu use picks the best user.
    - **MP Cost Rate:** multiplies skill MP costs.
    - **TP Charge Rate:** multiplies TP gained from taking damage and from skill or item TP gain.
    - **Physical Damage:** physical damage taken.
    - **Magic Damage:** magic damage taken.
    - **Floor Damage:** damage from damage tiles.
    - **Experience:** EXP gained, including reserve members when that option is on.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-014 — Buffs and debuffs
- **What:** Temporary stacked changes to parameters.
- **Behaviour:**
  - Add Buff or Add Debuff moves a parameter one level up or down, at 25% of the base value per level. It lasts 1–1000 turns (the Primer says 0–1000).
  - To move more than one level at once, the effect is added several times.
  - Remove Buff or Remove Debuff resets a buffed or debuffed parameter.
  - The Debuff Rate trait (0–1000%) scales the debuff success chance.
  - **Primer:**
    - The cap is 2 levels each way (±50%).
    - A buff and a debuff cancel each other.
    - Remove Buff does nothing when the parameter is debuffed.
  - Debuff success also uses Luck (FRM-015).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-015 — State application rates and Luck
- **What:** How likely a state is to land.
- **Behaviour:**
  - The **Add State** effect has a 0–1000% success value in the help (the Primer says 0–100%). Values over 100% make success more likely than the base.
  - The target's **State Rate** trait (0–1000%) scales the chance. The Primer says these traits multiply together.
  - **State Resist** makes the holder immune. Resisting Knockout means HP 0 does not knock the holder out.
  - **Attack State** traits add states to normal attacks. The help gives 0–1000%, and the Primer says these add together.
  - **Luck (Primer):** the chance is multiplied by 1 + 0.001 × (the user's luck − the target's luck), never below 0. This applies to both state and debuff chances.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-016 — TP system
- **What:** The third resource, alongside HP and MP.
- **Behaviour:**
  - Skills can cost **TP** (0–100), in addition to or instead of MP (0–999). A skill cannot be used without enough of either.
  - Each skill or item has a **TP Gain** value, granted to the user after a successful hit.
  - The **Gain TP** effect gives 0–100 TP.
  - The Change TP command uses 1–100. The Change Enemy TP command exists for enemies.
  - TP Charge Rate scales TP earned from damage and actions (Primer).
  - **Battle start:** TP resets each battle and starts at a random value. The Primer says 0–24.
  - **Preserve TP** carries TP over between battles.
  - **Show TP in Window** (System 1) shows or hides TP in the status window.
  - The formula for TP gained from taking damage is not verified.
  - The maximum TP is not stated explicitly; documented TP ranges never exceed 100.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-017 — Agility and speed uses
- **What:** Every documented effect of agility.
- **Behaviour:**
  - Agility sets action order, together with skill and item Speed and the Attack Speed trait.
  - It sets TPB fill speed, which is based on the square root of agility.
  - **Primer:** it is used to compare the party and troop for surprise and preemptive odds.
  - Its role in escape odds is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-018 — HP floor rules and death checks
- **What:** When HP may reach 0.
- **Behaviour:**
  - **Slip damage:** may kill only if Death by Slip Damage is on; otherwise HP floors at 1.
  - **Damage floor tiles:** may kill only if Death by Floor Damage is on; otherwise HP floors at 1.
  - The Change HP and Change Enemy HP commands have an **Allow Death** toggle; without it, HP floors at 1.
  - Reaching 0 HP adds the Knockout state, unless Knockout is resisted.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-FRM-019 — Effect repetition and the Effects list
- **What:** Non-damage effects a skill or item can apply.
- **Behaviour:**
  - There are 13 effect types:
    - Recover HP, Recover MP, Gain TP.
    - Add State, Remove State.
    - Add Buff, Add Debuff, Remove Buff, Remove Debuff.
    - Special Effect (Escape), Grow, Learn Skill (actors only), Common Event.
  - Several Common Event effects may be attached, and all of them run.
  - **Repeat** applies the whole application that many times.
  - The Primer says the Add State list includes a "Normal Attack" entry, which applies whatever states the user's normal attack would.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/blog/a-primer-on-database-traits-and-effects

### RM-FRM-020 — EXP curve
- **What:** The per-class experience requirements.
- **Behaviour:**
  - The curve has four inputs:
    - **Base:** overall scale; lowering it lowers every requirement.
    - **Extra:** a flat amount added at every level.
    - **Acceleration A:** how fast requirements grow as level rises.
    - **Acceleration B:** extra growth that matters mostly at high levels.
  - The editor shows the per-level and total requirements as a table and a graph.
  - **The exact formula is not verified.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-FRM-021 — Item and drop economics
- **What:** The numeric rules for money and loot.
- **Behaviour:**
  - The sell price is half the buy price. A price of 0 means the item cannot be sold.
  - Each enemy drop has a 1/N chance (N = 1..1000), doubled by Drop Item Double.
  - Gold rewards are doubled by Gold Double.
  - Inventory per item is 0–99.
  - The maximum party gold is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_04.html

---

## SAV — Save and load

### RM-SAV-001 — Manual save slots
- **What:** Player-chosen save files.
- **Behaviour:**
  - The player saves to one of **20** slots from the menu's Save command or the Open Save Screen event command.
  - Saving is possible only while save access is enabled (SAV-003).
  - A save plays the Save SE and shows a configurable save message.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-SAV-002 — Autosave
- **What:** Automatic saving on certain triggers.
- **Behaviour:**
  - The **topmost** save slot is reserved for autosave.
  - **Triggers:** the end of a battle and a player transfer between locations.
  - It lets players resume after the game was interrupted.
  - It is controlled by **Enable Autosave** in System 1, which is on by default per the preview blog.
  - The product page describes autosave as having its own separate slot.
  - Whether Change Save Access also blocks autosave is not verified.
  - Whether the player can save manually into the autosave slot is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-4-event-list-system-tab-options-new-ui-release-date-price ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-SAV-003 — Save access control
- **What:** A runtime switch for whether saving is allowed.
- **Behaviour:**
  - **Change Save Access** sets saving to Enable or Disable. The setting persists until the command runs again.
  - The Open Save Screen event command exists separately. Whether it bypasses a disabled save access is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html

### RM-SAV-004 — Loading (Continue)
- **What:** Resuming from a save file.
- **Behaviour:**
  - Title → Continue lists the saves. Choosing one resumes from it, plays the Load SE and shows the load message.
  - A load option inside the in-game menu is not documented. The menu offers Save only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

### RM-SAV-005 — Game ID and save portability
- **What:** A per-project identifier used to tell saves apart.
- **Behaviour:**
  - Each project has a **Game ID**, viewable and editable in System 2 → Advanced.
  - It separates one game's saves from another's.
  - For browser builds, saves carry over when the game is updated, even if the title changes.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html

### RM-SAV-006 — Contents of a save
- **What:** What a save file holds.
- **Behaviour:**
  - The help says only that it stores the "current game state".
  - Some persistent runtime settings are documented as lasting "until changed again". These include window colour, battle BGM, victory and defeat ME, actor and vehicle images, and the menu, save, formation and encounter access flags. These are game state and so would be saved, but the help does not explicitly list save contents. **The field list is not verified.**
  - The Save BGM / Resume BGM pair keeps a playback position (AUD-007). Whether that persists across a save file is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html

### RM-SAV-007 — Storage backend
- **What:** Where saves physically live.
- **Behaviour:**
  - **Not verified.** The official API docs contain no StorageManager or DataManager page.
  - The only related official API facts:
    - `Utils.canUseIndexedDB()` checks whether the browser supports IndexedDB.
    - `Utils.isNwjs()` and `Utils.isLocal()` detect the desktop wrapper and local file access.
  - No official page links these functions to saving.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

---

## AUD — Audio

### RM-AUD-001 — The four audio channels
- **What:** The audio categories MZ separates.
- **Behaviour:**
  - **BGM:** background music.
  - **BGS:** looping background sound such as ambience.
  - **ME:** short "music effect" stings. Victory, defeat and game-over music are MEs.
  - **SE:** one-shot sound effects.
  - Each has its own event commands and its own tab in the Sound Test tool.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-AUD-002 — Play BGM / BGS semantics
- **What:** Starting looped music or ambience.
- **Behaviour:**
  - Each play sets a file, volume, pitch and balance.
  - Choosing **(None)** stops the current track.
  - If the requested track is already playing, the command does nothing; it does not restart the track.
  - There is one BGM and one BGS at a time. "Play" replaces the current one.
  - Whether a new BGM crossfades is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html

### RM-AUD-003 — Play ME semantics
- **What:** Short musical cues.
- **Behaviour:**
  - Same parameters as BGM. **(None)** stops it, and the command is skipped if that ME is already playing.
  - It is used by the system for Victory, Defeat and Game Over.
  - Whether BGM pauses and resumes around an ME is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-AUD-004 — Play SE and Stop SE semantics
- **What:** Sound effects.
- **Behaviour:**
  - A new SE plays on top of any SE still sounding; they overlap.
  - Playing "(None)" does **not** stop running SEs. **Stop SE** halts all of them.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html

### RM-AUD-005 — Volume, pitch and balance parameters
- **What:** Per-play sound parameters.
- **Behaviour:**
  - **Volume:** the event command page gives no range. The Sound Test tool slider is 0–100%.
  - **Pitch:** 50–150%. Above 100% plays faster and higher.
  - **Balance (pan):** −100 to 100. Negative favours the left speaker and positive the right.
  - The runtime WebAudio object exposes matching `volume`, `pitch` and `pan` members.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/WebAudio.html

### RM-AUD-006 — Fade-out
- **What:** Gradual stop of BGM or BGS.
- **Behaviour:**
  - Fadeout BGM and Fadeout BGS lower the volume over 1–60 seconds and then stop.
  - There is no ME or SE fade command.
  - The API's `fadeIn(seconds)` and `fadeOut(seconds)` methods exist on WebAudio. When the engine uses fade-in automatically is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/WebAudio.html

### RM-AUD-007 — Save BGM / Resume BGM
- **What:** Remembering and restoring music.
- **Behaviour:**
  - **Save BGM** records the current BGM, including its playback position.
  - **Resume BGM** restarts the recorded BGM from that position.
  - Neither command takes parameters.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html

### RM-AUD-008 — System music assignments
- **What:** The music played automatically by scenes.
- **Behaviour:**
  - **System 1 Music:**
    - Title (BGM).
    - Battle (BGM).
    - Victory (ME).
    - Defeat (ME).
    - Game Over (ME).
    - Boat, Ship and Airship (BGM).
  - **Runtime overrides that persist:**
    - Change Battle BGM. If changed during a battle, it applies from the next battle.
    - Change Victory ME and Change Defeat ME.
    - Change Vehicle BGM (per vehicle).
  - Map music is set per map instead (AUD-010).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html

### RM-AUD-009 — System sound-effect assignments
- **What:** The full list of system SE slots.
- **Behaviour:**
  - Menu SEs: Cursor, OK, Cancel, Buzzer, Equip, Save, Load, Shop, Use Item, Use Skill.
  - Battle SEs: Battle Start, Escape, Enemy Attack, Enemy Damage, Enemy Collapse, Boss Collapse 1, Boss Collapse 2, Ally Damage, Ally Death, Recovery, Miss, Evasion, Magic Evasion, Magic Reflection.
  - The trigger for each slot is listed in MNU-019 and BTL-025.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html

### RM-AUD-010 — Map autoplay audio and music continuity
- **What:** Per-map ambience and music.
- **Behaviour:**
  - A map can autoplay a BGM and/or a BGS while the player is on it.
  - If the map BGM and the battle BGM are the same, the music continues uninterrupted when switching between map and battle. Both the Map Properties page and the Change Battle BGM page say this.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html

### RM-AUD-011 — Audio file format and looping
- **What:** Supported audio assets.
- **Behaviour:**
  - The format is **Ogg Vorbis (.ogg)**.
  - If the file metadata includes **LOOPSTART** and **LOOPLENGTH**, playback loops using those sample positions.
  - Assets live under the project's `audio` folder. Subfolders are allowed, and half-width alphanumeric names are recommended.
  - The official API has `Utils.canPlayOgg()` to check browser support.
  - The per-type subfolder names (for example `audio/bgm`) are not stated in the help text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_02.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-AUD-012 — Movies
- **What:** Video playback.
- **Behaviour:**
  - The Play Movie command plays a file.
  - Every movie should be provided as both **WebM** and **MP4 (H.264)** with the same base name. Windows and Mac builds need only WebM; web and mobile need both.
  - **API:**
    - `Video.play(src)`, `isPlaying()`, `resize()`.
    - `setVolume(0..1)`.
    - `Utils.canPlayWebm()`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Video.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-AUD-013 — Runtime audio object (WebAudio API)
- **What:** The core audio playback primitive documented in the official API.
- **Behaviour:**
  - The object is built from a file URL. It has `volume`, `pitch` and `pan` members.
  - **Methods:**
    - `play(loop, offset)`, where offset is the start position in seconds.
    - `stop()`, `seek()` (current position), `fadeIn(s)`, `fadeOut(s)`.
    - `isReady()`, `isPlaying()`, `isError()`, `retry()`.
    - Load and stop listeners.
  - `WebAudio.setMasterVolume(0..1)` sets the global volume.
  - The output requires Web Audio API support; see `Utils.canUseWebAudioAPI`.
  - The higher-level AudioManager (BGM/BGS/ME/SE routing, per-channel option volumes) is **not** in the official API docs, so it is **not verified**.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/WebAudio.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-AUD-014 — Audio encryption
- **What:** Optional encryption of audio at deploy time.
- **Behaviour:**
  - The deployment option "Sound Files" encrypts every audio file using a user-supplied key.
  - Encrypted files cannot be used inside the project folder.
  - The runtime checks with `Utils.hasEncryptedAudio()` and decrypts with `Utils.decryptArrayBuffer`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Utils.html

### RM-AUD-015 — Player volume options
- **What:** Player-adjustable volume.
- **Behaviour:**
  - The Options screen adjusts settings "such as BGM volume".
  - The full per-channel option list (BGS, ME, SE) is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

---

## MSGB — Message system behaviour

### RM-MSGB-001 — Message window basics
- **What:** The Show Text window.
- **Behaviour:**
  - It shows up to **4 lines** at a time.
  - **Position** is Bottom, Middle or Top.
  - It has a selectable **background** style; the option names are not verified in text.
  - The editor previews the window and draws a guide line for the maximum width.
  - **Batch Entry** splits long text into consecutive 4-line Show Text commands.
  - Text normally waits for player input after it finishes. This is implied by `\^`, which turns that wait off. Which buttons advance the text is not verified.
  - A pause indicator is part of the base Window object (`pause`, the "pause sign").
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Window.html

### RM-MSGB-002 — Face graphic
- **What:** A portrait inside the message window.
- **Behaviour:**
  - The face is drawn on the **left side** of the window.
  - Face sheets live in `img/faces`. Each face is 144x144, and a sheet holds 2 rows x 4 columns (8 faces).
  - The same faces are used in the menu.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MSGB-003 — Speaker name box
- **What:** A separate name label for the speaker. It is new in MZ.
- **Behaviour:**
  - The **Name** field shows a speaker label above the message.
  - Whether control characters work in the Name field is not verified; the help's list of places that accept them does not mention it.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-MSGB-004 — Substitution control characters
- **What:** Escape codes replaced by game data.
- **Behaviour:**
  - `\V[n]`: the value of variable n.
  - `\N[n]`: the name of actor n.
  - `\P[n]`: the name of the nth party member, in party order.
  - `\G`: the currency unit.
  - `\\`: a literal backslash.
  - All codes must be typed as half-width characters.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSGB-005 — Formatting control characters
- **What:** Escape codes that change how text is drawn.
- **Behaviour:**
  - `\C[n]`: switches to colour n. The palette comes from Window.png.
  - `\I[n]`: draws icon n from IconSet.png.
  - `\{` and `\}`: make text one step larger or smaller.
  - `\FS[n]`: sets the font size to n.
  - `\PX[n]` and `\PY[n]`: set the drawing x and y relative to the window's top-left.
  - PX, PY and FS are new in MZ.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html

### RM-MSGB-006 — Flow control characters
- **What:** Escape codes that change timing and window behaviour.
- **Behaviour:**
  - `\$`: opens the gold window.
  - `\.`: waits 1/4 second.
  - `\|`: waits 1 second.
  - `\!`: waits for a button press.
  - `\>`: draws the rest of the line instantly.
  - `\<`: ends instant drawing.
  - `\^`: closes without waiting for input after the text finishes.
  - One frame is 1/60 second, per the Wait command.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_09.html

### RM-MSGB-007 — Where control characters work
- **What:** Other text fields that accept codes.
- **Behaviour:**
  - The codes `\V \N \P \G \C \I \{ \} \\ \PX \PY \FS` also work in:
    - Show Choices.
    - Actor Profile.
    - Skill Message.
    - Item, Weapon and Armor Descriptions.
    - State Messages.
    - The Terms Save and Load messages.
    - The Terms battle messages from "Party Name" on.
  - In those battle messages, `\$ \. \| \! \> \< \^` also work.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSGB-008 — Choices window
- **What:** A branching menu shown with or after a message.
- **Behaviour:**
  - It holds up to **6** choices. Blank choices are hidden. Control characters are allowed.
  - Settings: background type, screen position, the cursor's starting item, and what Cancel does.
  - The editor creates one branch per choice, plus a cancel branch.
  - The specific Cancel options (disallow, branch, or pick a choice) are not verified in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSGB-009 — Number input window
- **What:** Numeric entry stored in a variable.
- **Behaviour:**
  - The input has 1–8 digits. The result goes into the chosen variable.
  - The per-digit controls are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSGB-010 — Select Item window
- **What:** An item picker that stores the chosen item's ID in a variable.
- **Behaviour:**
  - It shows one item type: the regular, key or hidden A/B categories.
  - Hidden items appear only here.
  - The value stored on cancel is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-MSGB-011 — Scrolling text
- **What:** Credits-style text rising from the bottom of the screen.
- **Behaviour:**
  - Speed is 1–8, and higher is faster.
  - Pressing **confirm** fast-forwards the scroll unless **No Fast Forward** is set. Whether it must be held is not verified.
  - The editor provides an icon viewer.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-MSGB-012 — Base window object behaviour
- **What:** The documented core Window class that every UI window builds on.
- **Behaviour:**
  - **Openness** runs from 0 (closed) to 255 (open), which supports open and close animations.
  - Separate opacities for the frame and background, the background alone, and the contents.
  - A cursor rectangle and cursor visibility.
  - Up and down scroll arrows.
  - The pause sign.
  - Padding and margin.
  - A scroll origin.
  - A windowskin bitmap.
  - **`setTone(r,g,b)`** takes −255..255 per channel, matching Window Color.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Window.html

### RM-MSGB-013 — Fonts and text size
- **What:** Typography settings.
- **Behaviour:**
  - The main font file, a separate number font and a fallback font are set in System 2.
  - The base font size is also set in System 2.
  - In-text size changes use `\{`, `\}` and `\FS[n]`.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

---

## INP — Input

### RM-INP-001 — Logical actions and default bindings
- **What:** The default control scheme, based on a 4-button pad labelled like an Xbox 360 controller.
- **Behaviour:**

  | Action | Gamepad | Keyboard | Mouse | Touch |
  |---|---|---|---|---|
  | Move | D-pad | Arrow keys, numpad | Click destination | Tap destination |
  | OK | A | Z, Enter, Space | Click target | Tap target |
  | Cancel | B | X, Esc, Numpad 0 | Right click | Two-finger tap / cancel button |
  | Menu | Y | X, Esc, Numpad 0 | Right click | Two-finger tap / menu button |
  | Dash | X | Shift | — | — |
  | Previous page | LB | Q, Page Up | — | — |
  | Next page | RB | W, Page Down | — | — |
  | Scroll pages | D-pad | Arrow keys, numpad | Drag from inside to outside, or the wheel | Hold and swipe from inside to outside |

  - On keyboard and mouse, Menu and Cancel share the same inputs; only the gamepad separates them.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-INP-002 — Click and tap movement
- **What:** Pointer-driven walking on the map.
- **Behaviour:**
  - Clicking or tapping a spot moves the player there.
  - Holding the press inside the screen keeps the player moving in that direction.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-INP-003 — Function keys
- **What:** Built-in utility keys.
- **Behaviour:**
  - **F2:** toggle the FPS and drawing-mode display.
  - **F3:** toggle stretch mode.
  - **F4:** toggle full screen.
  - **F5:** reload.
  - **F8:** developer tools (playtest only).
  - **F9:** debug screen (playtest only).
  - **Ctrl + arrow:** walk through obstacles (playtest only).
  - **Wheel:** scroll pages.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-INP-004 — Pressed, triggered and repeated semantics
- **What:** The three ways a button state is read.
- **Behaviour:**
  - **Pressed:** true the whole time the button is held.
  - **Triggered:** true only on the first frame of the press.
  - **Repeated:**
    - true on the first frame;
    - true again **24 frames** later;
    - then true every **6 frames** while held.
  - Events can branch on any of these with Conditional Branch → Button.
  - The API also offers a long-press check.
  - The API exposes `keyRepeatWait` and `keyRepeatInterval` in frames. The 24/6 figures come from the help.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_03.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Input.html

### RM-INP-005 — Keyboard and gamepad input API
- **What:** The official Input static class.
- **Behaviour:**
  - It handles input from the keyboard and gamepads.
  - **Mapping tables:**
    - `keyMapper` maps virtual key codes to logical key names.
    - `gamepadMapper` maps pad buttons to logical names.
  - **Directions:**
    - `dir4` gives the 4-way direction as a numpad digit, 0 when neutral.
    - `dir8` gives the 8-way direction, also as numpad digits.
  - **Queries:** `isPressed`, `isTriggered`, `isRepeated` and `isLongPressed`, each taking a logical name.
  - **Housekeeping:** `date` (time of the last input), `update` and `clear`.
  - The exact logical name strings and the full key-code table are not listed in the docs; **not verified**.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/Input.html

### RM-INP-006 — Mouse and touch input API
- **What:** The official TouchInput static class.
- **Behaviour:**
  - It reads the mouse and the touchscreen. It reports `x` and `y` in canvas coordinates, plus `wheelX` and `wheelY`.
  - **Queries:**
    - `isTriggered`, `isPressed`, `isRepeated` (with a pseudo key repeat) and `isLongPressed` cover the left button or touch.
    - `isReleased` covers the left button or touch being let go.
    - `isClicked` means pressed and released at the same spot.
    - `isCancelled` means the right button was just pressed.
    - `isMoved` reports movement, and `isHovered` reports mouse movement without a button held.
  - **Settings:** `moveThreshold` (pixels that count as movement) and pseudo key-repeat wait and interval.
  - Page-to-canvas conversion is in `Graphics.pageToCanvasX/Y`, and `Graphics.isInsideCanvas` tests a point.
- **Source:** https://developer.rpgmakerweb.com/rpg-maker-mz/TouchInput.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Graphics.html

### RM-INP-007 — Touch UI affordances
- **What:** Touch-specific behaviour in the default game.
- **Behaviour:**
  - On-screen buttons from ButtonSet.png handle Menu, Cancel or Back, and shop quantity.
  - A two-finger tap means cancel or menu.
  - Swipe scrolling works in windows.
  - Battle targets can be selected by touch.
  - The MZ "conditional branch" adds touch and mouse conditions, including long press.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html

### RM-INP-008 — Context-specific bindings
- **What:** Screens that reuse inputs in their own way.
- **Behaviour:**
  - **Shop quantity:** Left/Right change it by ∓1 and Up/Down by ±10.
  - **Shop member window:** Dash pages through members beyond the fifth.
  - **TPB:** Cancel switches between ready actors.
  - **Debug screen:** Left/Right or L/R change variables (DBG-002).
  - **Name input:** the D-pad moves the cursor, OK enters a character and delete removes one.
  - **Scrolling text:** OK fast-forwards.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-5-tpbs-a-closer-look ; https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html

### RM-INP-009 — Dash
- **What:** Faster walking.
- **Behaviour:**
  - Hold **Shift** (keyboard) or **X** (gamepad) to dash.
  - Individual maps can disable dashing with **Disable Dashing**.
  - An "Always Dash" player option is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html

---

## GEN — Character Generator

### RM-GEN-001 — Access and body-type tabs
- **What:** The editor tool for assembling character art from parts.
- **Behaviour:**
  - It opens from Tools → Character Generator.
  - It has three base tabs: **Male**, **Female** and **Kid**.
  - A live **Preview** shows the current combination.
  - The output art can be assigned to actors and events.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html

### RM-GEN-002 — Parts: variation, colour, offset
- **What:** Per-part customization.
- **Behaviour:**
  - Each feature has a **variation** (which art), a **colour** and an **offset**.
  - The **offset**, new in MZ, nudges a part up, down, left or right. The blog's examples are bunching facial features, adjusting how glasses sit, and moving a hat to fit the hair.
  - The product page lists hair, eyes, clothes and accessories as examples of parts.
  - The full part list for each base tab is not verified in text.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-GEN-003 — Colour palettes and gradients
- **What:** The recolouring system.
- **Behaviour:**
  - There is a colour palette per part.
  - New colour gradients can be added by appending a row to the generator's gradient PNG. The generator then offers the new row as an option, so the number of gradients has no fixed limit.
  - The blog says gradients apply to eyes, clothes, accessories, hair and skin.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_05.html ; https://www.rpgmakerweb.com/blog/rpg-maker-mz-preview-3-character-generator-plugin-manager-event-plugin-command ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-GEN-004 — Randomize
- **What:** A one-click random character.
- **Behaviour:** It sets every part to a random choice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-GEN-005 — Output types
- **What:** The image kinds the generator exports.
- **Behaviour:** Separate export windows exist for:
  - the **Face Image**;
  - the **Walking Character** (map sprite);
  - the **Damaged Character**;
  - the **Battler** (side-view battler).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

### RM-GEN-006 — Export window: Import / Export
- **What:** Writing generated art to files.
- **Behaviour:**
  - **Import** opens an existing sheet from disk and places the character just generated into it.
  - **Export** saves the image to a file.
  - The generator page does not state output sizes. For reference, the asset standards define faces as 144x144 cells (2x4 per sheet) and SV battlers as 9x6 cells (576x384 standard).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_11_02.html

### RM-GEN-007 — Save / Load settings
- **What:** Persisting a character recipe.
- **Behaviour:**
  - **Save Settings** stores the current part, colour and offset choices.
  - **Load Settings** restores them.
  - The settings file format is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

---

## DBG — Debug and test

### RM-DBG-001 — Playtest
- **What:** Running the game from the editor.
- **Behaviour:**
  - Start it with Game → Playtest. It runs the game as it will ship, so settings and events can be checked.
  - Some features work only in playtest: F8, F9 and Ctrl-move.
  - Close the window to quit.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DBG-002 — Debug screen (F9)
- **What:** An in-game editor for switches and variables.
- **Behaviour:**
  - **Opening and closing:** F9 during a playtest opens it. F9 or Cancel returns to the game.
  - **Left list:** choose S (switches) or V (variables) and a range of IDs.
  - **Selecting:** OK moves the cursor into the entry list on the right.
  - **Editing:**
    - Switches flip ON/OFF with the OK button.
    - Variables change with Left/Right or the L/R buttons.
  - The step sizes for variable changes are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DBG-003 — Walk through walls
- **What:** Collision bypass for testing.
- **Behaviour:**
  - Holding **Ctrl** while moving during a playtest lets the player pass impassable tiles.
  - It is playtest-only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DBG-004 — Developer tools (F8)
- **What:** Browser-style inspector.
- **Behaviour:**
  - **F8** during a playtest opens developer tools equivalent to Chrome's. They let the developer inspect the HTML5 game.
  - It is playtest-only.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_06.html

### RM-DBG-005 — Display and diagnostic toggles
- **What:** Runtime keys useful while testing.
- **Behaviour:**
  - **F2:** FPS and drawing-mode overlay.
  - **F3:** stretch mode.
  - **F4:** full screen.
  - **F5:** reload the game.
  - The help does not mark F2–F5 as playtest-only.
  - **Error display:** the API documents `Graphics.printError`, which shows error text on screen, and `Graphics.showRetryButton`, which offers to reload failed resources.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_06.html ; https://developer.rpgmakerweb.com/rpg-maker-mz/Graphics.html

### RM-DBG-006 — Battle Test
- **What:** Running a single troop battle from the database.
- **Behaviour:**
  - It is launched from Troops → Battle Test.
  - Tabs **1–4** set up each party slot: actor, equipment and level. A Status panel shows the resulting parameters.
  - OK opens a window and starts the battle. Closing the window ends the test.
  - The project must be saved first for plugin settings to apply.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html

### RM-DBG-007 — Sound Test tool
- **What:** Previewing audio inside the editor.
- **Behaviour:**
  - It opens from Tools → Sound Test.
  - It has tabs for BGM, BGS, ME and SE, with Play/Stop and the space bar to play.
  - Sliders: Volume 0–100%, Pitch 50–150%, Balance −100..100.
  - Music keeps playing after the window closes.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_05.html

---

## Not verified

These items were looked for on official pages and **not** found. Do not implement them from these notes; they need another source or a design decision.

1. Scene_\*, Window_\* subclasses, Game_\*, BattleManager, AudioManager, DataManager, StorageManager, ConfigManager, SceneManager and ImageManager. **None appear in the official API docs**, which cover only the core layer; the probed URLs returned 404. Any class-level behaviour from these is unverified.
2. The full Options screen item list. Only BGM volume is named; Always Dash, Command Remember, Touch UI, and BGS/ME/SE volumes are not stated.
3. Message fast-forward or skip by holding a key during Show Text; a text-speed option; whether the message window advances with Cancel; the auto-advance timing of `\^`.
4. The message background option names (window, dim, transparent) and the Show Choices cancel options. They appear only in screenshots.
5. **Escape success ratio formula**, and whether failed attempts raise it.
6. The turn-based action-order formula and any random speed variance.
7. The critical damage multiplier.
8. The exact hit and evasion formula: the order of success rate, hit rate and evasion rolls.
9. How several attack elements combine for normal attacks.
10. The full damage pipeline order (formula → element → physical/magic rate → critical → variance → guard), and behaviour with negative formula results.
11. TP gained from taking damage (formula). TP maximum: implied 100 but never stated.
12. The EXP curve formula from Base, Extra, Acceleration A and Acceleration B.
13. The surprise and preemptive formula: help gives no numbers, only the Primer gives percentages, and its "Raise Preemptive" increment is ambiguous.
14. Which skill or item field holds the TPB "casting time". The preview blog says one exists; the help shows no such field.
15. TPB behaviour details beyond the blog: what happens to the gauge on stun, whether the timer runs during animations, escape under TPB.
16. Save file contents (field list), the storage backend and location, the save file format, and whether manual saves can overwrite the autosave slot.
17. Whether Change Save Access also disables autosave, and whether Open Save Screen works while saving is disabled.
18. The default Status screen layout and the Equip screen parameter-comparison display.
19. The name-input character table layout and the key that deletes a character.
20. The step sizes for editing variables on the debug screen.
21. The maximum party gold. Shop purchase behaviour at the 99-item cap.
22. Per-type audio subfolder names (bgm/bgs/me/se) in help text, BGM crossfade behaviour, and whether BGM pauses and resumes around an ME.
23. Logical key-name strings, the full keyboard key-code map and gamepad button indices. The docs describe `keyMapper` and `gamepadMapper` but not their contents.
24. The side-view motion name list and grid positions, which are shown only as an image.
25. The full part list for each Character Generator tab and the generator settings file format.
26. Victory and level-up result display layout and message wording (in Terms, screenshot only).
27. The behaviour of Damage Type "None".
28. The timer's behaviour during battle.
29. Range conflicts between the help and the Primer. The help value is listed first:
    - Ex-Parameter trait: −100..100% vs −1000..1000%.
    - Attack Speed: −999..999 vs −1000..1000.
    - Add State: 0–1000% vs 0–100%.
    - Buff turns: 1–1000 vs 0–1000.
    - Recover HP flat: ±999,999 vs ±99,999.
