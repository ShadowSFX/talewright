# RPG Maker MZ Feature Inventory: Database (part A)

Database overview, Actors, Classes, Skills, Items, Weapons, Armors.

Clean-room note: every item below is written in our own words from the official RPG Maker MZ English help, retrieved on 2026-10-13 with WebFetch. Field names are kept as the editor shows them. Nothing here is copied prose, and there is no code. Where a page does not state something, the item says "not verified" rather than filling the gap from memory.

Method note: WebFetch returns text processed by the fetch tool, not raw HTML. Where the first summary of a page was ambiguous or contradicted itself, the page was fetched again with narrow questions and the exact wording of the relevant fields was checked before writing. Two such contradictions were found (Skill Speed direction and the meaning of Skill Type "None"). Both are recorded as not verified below.

## 1. Pages covered

| Page | URL | Load status | Use |
|---|---|---|---|
| Database (overview) | https://rpgmakerofficial.com/product/MZ_help-en/01_08.html | Loaded (fetched 2x) | Primary |
| Actor Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html | Loaded (fetched 3x) | Primary |
| Class Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html | Loaded (fetched 2x) | Primary |
| Skill Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html | Loaded (fetched 4x) | Primary |
| Item Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html | Loaded (fetched 3x) | Primary |
| Weapon/Armor Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html | Loaded (fetched 3x) | Primary |
| Terms Settings (linked from Skills, Weapons/Armors) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html | Loaded | Cross-reference only |
| How to Set Traits (linked from Actors, Classes, Weapons/Armors) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html | Loaded | Cross-reference only (trait names and the editor workflow); the full trait inventory belongs to another slice |
| How to Set Effects (linked from Skills, Items) | https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html | Loaded | Cross-reference only (effect names and the editor workflow); the full effect inventory belongs to another slice |

No page failed to load. Each primary page shows a full-context link label, but the fetched text gave no target for it, so it was not followed.

---

## 2. Items

### RM-DB-001 — Database scope and data categories
- **What:** The Database is the editor that holds all game data except maps and the events placed on maps.
- **Behaviour:**
  - The overview says there are 15 data types, but the list has 16 entries because System appears as two tabs. In page order: Actor, Classes, Skills, Items, Weapons, Armor, Enemies, Troops, States, Animations, Tilesets, Common Events, System 1, System 2, Type, Terms.
  - Actor through Common Events are lists of individual entries, each with its own settings. The user creates as many entries as needed, for example a separate item entry per potion or key.
  - System 1 and System 2 hold game-wide settings: System 1 the basic ones, System 2 the advanced ones.
  - Type defines the type lists for elements, skills, weapons, armors and equipment.
  - Terms holds the display names of commands, parameters and similar UI text.
  - System, Type and Terms are single settings pages, not lists of entries.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-002 — Opening the Database editor
- **What:** Ways to open the Database window.
- **Behaviour:**
  - From the Database button on the main toolbar.
  - From the Tools menu, under Database.
  - No other entry points (such as a shortcut key) are given.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-003 — Edit screen layout and entry navigation
- **What:** How the Database window is laid out and how the user moves between entries.
- **Behaviour:**
  - A side menu selects the data type (tab).
  - For list-based types, a list of entries is on the left. Selecting an entry shows its parameters in the edit area on the right.
  - F4 and F5 step to the previous and next entry in the list. The page does not say which key goes which way.
  - The page mentions no OK, Cancel, Apply, save or undo behaviour for the Database window.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-004 — Change Maximum (entry count per type)
- **What:** Sets how many entries (ID slots) a data type has.
- **Behaviour:**
  - A "Change Maximum" button sits below the entry list. Clicking it lets the user type a new count.
  - Upper limits: Skills, Items, Weapons, Enemies and Troops allow up to 2000 entries. Every other list type allows up to 1000.
  - Lowering the maximum deletes every entry whose ID is above the new limit.
  - Whether a confirmation prompt appears: not verified (the page does not mention one).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-005 — Entry context menu (Copy / Paste / Clear)
- **What:** Right-click operations on entries in the data list.
- **Behaviour:**
  - **Copy:** puts the selected entry's full parameter set on the clipboard. Holding Shift while selecting lets several entries be copied at once.
  - **Paste:** replaces the target entry's parameters with the clipboard contents. The entry's previous settings are lost.
  - **Clear:** wipes all of the entry's parameters.
  - The page does not say whether multi-selection works with Paste or Clear, or whether Ctrl selection is supported.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-006 — Data IDs
- **What:** Each entry has a unique number, shown at the start of its row in the list.
- **Behaviour:**
  - Each ID is unique within its data type.
  - Event commands use the ID to refer to an entry. The ID can also be supplied indirectly through a game variable.
  - The ID display format (for example zero padding) is not described.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html

### RM-DB-007 — Notes field (shared)
- **What:** A free-text field on most entry types, for the developer's own notes and for plugin data.
- **Behaviour:**
  - Present on Actors, Classes, Skills, Items, Weapons, Armors, Enemies, States and Tilesets.
  - It normally has no effect on the game. Plugins may read it to define extra data.
  - On the Actor, Class, Skill, Item and Weapon/Armor pages, right-clicking the field opens a menu with a "Plugin Help..." command that shows plugin help information.
  - The Actor page labels this field "Memo". The overview and the other pages call it Notes.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-DB-008 — Traits editor (shared mechanism, cross-reference)
- **What:** A list of traits used by Actors, Classes, Weapons, Armors, Enemies and States to grant properties.
- **Behaviour:**
  - Add a trait by double-clicking an empty row, choosing a trait kind and setting its values.
  - Edit a trait by double-clicking its row.
  - Copy or delete a trait from the right-click menu.
  - There are 25 trait kinds on six tabs:
    - Rate: Element Rate, Debuff Rate, State Rate, State Resist.
    - Param: Regular Parameters, Ex-Parameters, Sp-Parameter.
    - Attack: Attack Element, Attack State, Attack Speed, Attack Times, Attack Skills.
    - Skill: Add Skill Type, Seal Skill Type, Add Skill, Seal Skill.
    - Equip: Equip Weapon, Equip Armor, Lock Equip, Seal Equip, Slot Type.
    - Other: Action Times +, Collapse Effect, Special Flag, Party Ability.
  - Regular-parameter traits change a parameter by a percentage, from 0 to 1000%, where 100% means no change.
  - Slot Type has a single option, Dual Wield. It allows two weapons but removes the shield slot.
  - Several Action Times + entries are each checked against their own chance.
  - The full detail of every trait belongs to the slice that covers 01_08_15.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-DB-009 — Effects editor (shared mechanism, cross-reference)
- **What:** A list of non-damage effects that Skills and Items apply on use. Several effects can be combined on one entry.
- **Behaviour:**
  - Add an effect by double-clicking an empty row, choosing the effect kind and setting its target, strength and options.
  - Edit an effect by double-clicking its row.
  - Copy or delete an effect from the right-click menu.
  - There are 13 effect kinds on four tabs:
    - Recover: Recover HP, Recover MP, Gain TP.
    - State: Add State, Remove State.
    - Param: Add Buff, Add Debuff, Remove Buff, Remove Debuff.
    - Other: Special Effect, Grow, Learn Skill, Common Events.
  - The full detail of every effect belongs to the slice that covers 01_08_16.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-DB-010 — Terms page (cross-reference for type and parameter names)
- **What:** The Skill and Weapon/Armor pages point to Terms for renaming skill, weapon and armor types. The Terms page itself describes renaming built-in vocabulary.
- **Behaviour:**
  - Terms covers:
    - Basic Statuses: Level, HP, MP, TP and EXP, each with an optional abbreviation used in places such as the battle status window.
    - Parameter names.
    - Equipment Type (slot) names.
    - Command names.
    - In-game message templates, which use %1 and %2 style placeholders.
  - The fetched Terms page did not list skill types, weapon types, armor types or elements. Those lists appear to live on the separate Type tab (see RM-DB-001), which is not in this slice.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_14.html

---

### RM-ACT-001 — Actor data role
- **What:** Actors are the player-controlled characters. Each one can carry its own traits.
- **Behaviour:**
  - The Actor page has these sections: Basic Settings, Images, Initial Equipment, Traits, Memo.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-002 — Name
- **What:** The actor's display name in the game.
- **Behaviour:**
  - A long name may be cut off in menus and in battle.
  - No character limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-003 — Nickname
- **What:** A secondary title for the actor.
- **Behaviour:**
  - Shown in the upper-right area of the status screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-004 — Class
- **What:** The class the actor belongs to.
- **Behaviour:**
  - The class affects which skills the actor can use, which weapons it can equip, which items it can use, and similar things.
  - Class details are edited on the Classes tab (see RM-CLS-*).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-005 — Initial Level / Max Level
- **What:** The actor's starting level and its level cap.
- **Behaviour:**
  - Both values range from 1 to 99.
  - The actor can never level up past its Max Level.
  - What happens if Initial Level is set above Max Level: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-006 — Profile
- **What:** Introductory text about the actor.
- **Behaviour:**
  - Shown at the bottom of the status screen.
  - No line limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-007 — Face image
- **What:** The actor's face graphic.
- **Behaviour:**
  - Used on the menu screen.
  - How the image is picked, and its size, are not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-008 — Walking Character image
- **What:** The actor's sprite on the map.
- **Behaviour:**
  - Used on the Map Screen.
  - How the image is picked, and its size, are not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-009 — [SV] Battler image
- **What:** The actor's battle graphic.
- **Behaviour:**
  - Used on the Battle Screen only when the game is in Side-View battle mode.
  - What is shown for actors in front-view battles is not described.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-010 — Initial Equipment
- **What:** The gear the actor has equipped when the game starts.
- **Behaviour:**
  - Each equipment slot has a drop-down list for choosing one item.
  - The list only offers equipment that the actor's class can equip.
  - Choosing None leaves that slot empty.
  - The number and names of slots are not listed on this page. They come from equipment types; see RM-ARM-006.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

### RM-ACT-011 — Traits
- **What:** Traits specific to this actor.
- **Behaviour:**
  - Each trait is set up in a detail window opened by double-clicking a row of the traits list.
  - The trait kinds are listed in RM-DB-008.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-ACT-012 — Memo (Notes)
- **What:** The developer's notes for the actor.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help..." to show plugin help. See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_01.html

---

### RM-CLS-001 — Class data role
- **What:** A class defines an actor's abilities. Every actor must belong to one.
- **Behaviour:**
  - The class controls how EXP and level-ups work, how parameters grow with level, and which skills are learned.
  - Classes can carry their own traits.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-002 — Name
- **What:** The class's name.
- **Behaviour:**
  - Shown on the menu screen and at the top of the status screen.
  - No length limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-003 — EXP Curve parameters
- **What:** Four numbers that together set how much EXP each level requires.
- **Behaviour:**
  - **Base Value:** the core figure the required EXP is built from. Lowering it lowers the EXP needed across the board.
  - **Extra Value:** a fixed amount added to the requirement at every level.
  - **Acceleration A:** sets how fast the requirement grows. Raising it makes the requirement climb in proportion to level.
  - **Acceleration B:** also sets the growth rate, but its effect is concentrated at higher levels. Raising it steepens the curve mostly toward the top levels.
  - Ranges and defaults for the four values are not stated.
  - The formula that combines them is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-004 — EXP Curve preview window
- **What:** A window, opened from the "..." button next to the EXP Curve values, for editing the curve and seeing its result.
- **Behaviour:**
  - It has two tabs:
    - **To Next Level:** lists the EXP needed to go from each level to the next, with a graph behind the list. The help recommends watching this tab while adjusting the values.
    - **Total:** lists the cumulative EXP needed to reach each level.
  - The level range shown in this window is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-005 — Parameter Curves editor window
- **What:** A per-level value table for each base parameter, edited in a settings window.
- **Behaviour:**
  - Opened by double-clicking the parameter graph on the Class page.
  - Each parameter has its own tab in the window, named after the parameter.
  - OK applies the changes. Cancel throws them away.
  - The Class page names only Max HP and Max MP; the others are called "all others". The tab names are not verified. The Skill page's formula help (RM-SKL-019) lists eight parameters: Max HP, Max MP, Attack, Defense, Magic Attack, Magic Defense, Agility and Luck. It is an inference, not stated on the Class page, that these are the eight tabs.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-006 — Parameter Curves: Quick Settings (presets A to E)
- **What:** One-click presets that fill every level's value for the current parameter.
- **Behaviour:**
  - There are five preset patterns, A, B, C, D and E, each with its own button.
  - Clicking a button applies that pattern to all levels.
  - The actual values or shape of each preset are not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-007 — Parameter Curves: Level/Value direct edit
- **What:** Manual entry of one level's value.
- **Behaviour:**
  - Choose a level from 1 to 99, then type its value.
  - Value ranges:
    - Max HP: 1 to 9999.
    - Max MP: 0 to 9999.
    - Every other parameter: 0 to 999.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-008 — Parameter Curves: Generate Curve
- **What:** A tool that fills in every level automatically from two end values.
- **Behaviour:**
  - The Generate Curve button opens a window where the user enters the level 1 value and the level 99 value:
    - Max HP: 1 to 9999.
    - Max MP: 0 to 9999.
    - Other parameters: 1 to 999. Note that this minimum is 1, while direct editing allows 0.
  - A growth-type slider shapes the curve between the two ends. Its left side is labelled early growth and its right side late growth. The help says growth gets slower toward the left and faster toward the right.
  - OK writes the generated values for all levels.
  - The interpolation formula is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-009 — Parameter Curves: Graph
- **What:** A bar chart of the parameter's value at every level.
- **Behaviour:**
  - Clicking or dragging on the graph reads out the value at the level under the cursor.
  - Whether dragging also edits values is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-010 — Skills to Learn (learnable skills)
- **What:** The skills actors of this class gain when they reach given levels.
- **Behaviour:**
  - Double-clicking the list opens a window for setting a level and a skill.
  - Each entry also has a Notes text field for the developer's reference.
  - Whether the list can hold several skills per level, and what happens to skills of levels already passed, are not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

### RM-CLS-011 — Traits
- **What:** Traits applied to every actor of this class.
- **Behaviour:**
  - Edited with the shared traits editor (RM-DB-008).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-CLS-012 — Notes
- **What:** The developer's notes for the class.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help". See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html

---

### RM-SKL-001 — Skill data role
- **What:** Skills are the actions taken in battle, including the plain attack, guarding, magic and special moves.
- **Behaviour:**
  - A skill sets when it can be used, its use conditions, its success rate, the damage it deals to targets, and so on.
  - Whether skill IDs 1 and 2 (Attack and Guard) have special roles: not verified (the page does not mention it).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-002 — Name
- **What:** The skill's display name.
- **Behaviour:**
  - A long name may be cut off on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-003 — Icon
- **What:** A small image shown next to the skill name during play.
- **Behaviour:**
  - Double-clicking the field opens the "Select an Image" window.
  - The help suggests picking an icon that fits the skill.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-004 — Description
- **What:** Help text for the skill.
- **Behaviour:**
  - Shown during play while the cursor is on the skill.
  - No line limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-005 — Skill Type
- **What:** The category the skill belongs to.
- **Behaviour:**
  - The default types are Magic and TP Skills. The help says they can be renamed through Terms.
  - A skill with any type other than None can only be used by actors whose actor or class has the matching Add Skill Type trait.
  - The page does not state outright what the None type allows. By implication it is not restricted by that trait, but this is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-006 — MP Cost / TP Cost
- **What:** The resource cost of using the skill.
- **Behaviour:**
  - MP Cost ranges from 0 to 999.
  - TP Cost ranges from 0 to 100.
  - An actor without enough MP or TP cannot use the skill.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-007 — Occasion
- **What:** Where the skill can be used.
- **Behaviour:**
  - Always: in battle and from the menu screen.
  - Battle Screen: in battle only.
  - Menu Screen: from the menu screen only.
  - Never: cannot be used anywhere.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-008 — Scope (Faction / Number / Status)
- **What:** Who the skill targets, set through three fields that must all have a value.
- **Behaviour:**
  - **Faction:** Enemy, Ally, Enemy & Ally, User, or Unspecified.
  - **Number:** a single target (1 Enemy/Ally), all of them (All Enemies/Allies), or Random.
  - **Status:** Alive, Knocked Out, or No Condition. It can only be set when the targets are allies.
  - How many targets Random picks, and how Enemy & Ally and Unspecified behave, are not described.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-009 — Speed (Invocation)
- **What:** An action-order modifier.
- **Behaviour:**
  - An integer from -2000 to 2000, added to the user's Agility when the skill is used.
  - It affects turn order in battle. The help frames it as a design trade-off: a skill can be weak but quick, or strong but slow.
  - The help does not say which sign (positive or negative) makes the skill act earlier: not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-010 — Success
- **What:** The skill's base chance of success.
- **Behaviour:**
  - Ranges from 0 to 100%.
  - The final chance is also affected by how effective the skill is against the target. How the two combine is not described.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-011 — Repeat
- **What:** How many times the effect is applied per use.
- **Behaviour:**
  - Ranges from 1 to 9.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-012 — TP Gain
- **What:** TP the user gains when the skill takes effect.
- **Behaviour:**
  - Granted after the effect is applied to a target on a successful use.
  - The range is not stated.
  - Whether it is granted per target or per repeat is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-013 — Hit Type
- **What:** How hitting is decided and which reactive mechanics apply.
- **Behaviour:**
  - **Certain Hit:** if the use succeeds, it always hits. Counterattack, Magic Reflection and Substitute have no effect on it.
  - **Physical Attack:** whether it hits depends on the user's Accuracy against the target's Evasion. Counterattack and Substitute can apply.
  - **Magic Attack:** success is checked against Magic Evasion of the receiver; the Magic Reflection and Substitute mechanics may intervene.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-014 — Animation
- **What:** The battle animation played when the skill is used.
- **Behaviour:**
  - The page lists no options. Special choices such as "Normal Attack" are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-015 — Message
- **What:** A fixed battle-log phrase shown when the skill is used.
- **Behaviour:**
  - Up to 2 lines.
  - The placeholder %1 is replaced with the user's name and %2 with the skill's name.
  - Three preset buttons insert ready-made phrases: "casts *!", "does *!" and "used *!".
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-016 — Required Weapon
- **What:** Weapon types that must be equipped before the skill can be used.
- **Behaviour:**
  - Two fields, Weapon Type 1 and Weapon Type 2.
  - If both are None, no weapon is needed.
  - If two types are set, a weapon of either type is enough.
  - Whether this applies to enemies is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-017 — Damage: Type
- **What:** What the skill's formula does to HP or MP.
- **Behaviour:**
  - One of six choices, all acting on HP or MP.
  - The help describes three behaviours:
    - Damage lowers the value.
    - Recover raises the value.
    - Drain subtracts from the target and adds that same quantity to the caster.
  - The exact names of all six options are not verified.
  - Whether a "None" option exists is not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-018 — Damage: Element
- **What:** The element attached to the damage.
- **Behaviour:**
  - The available choices (for example a "Normal Attack" or "None" entry) are not listed on the page.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-019 — Damage: Formula
- **What:** A typed expression that computes the base amount of the effect.
- **Behaviour:**
  - **Subjects:** the letter a stands for the user (attacker) and b for the target. A property is read by writing the subject letter, a dot, then the property name. For example, the user's attack power is "a" followed by ".atk".
  - **Readable properties:**
    - atk: Attack Power.
    - def: Defense.
    - mat: Magic Attack.
    - mdf: Magic Defense.
    - agi: Agility.
    - luk: Luck.
    - mhp: Max HP.
    - mmp: Max MP.
    - hp: current HP.
    - mp: current MP.
    - tp: current TP.
    - level: current level.
  - **Game variables:** v with the variable number in square brackets returns that variable's value.
  - **Operators:** the four basic arithmetic operators (add, subtract, multiply, divide).
  - **Examples the help describes:**
    - Reading the user's attack power on its own.
    - Four times the user's attack power minus twice the target's defense.
  - **Applied outside the formula:** elemental rates and the effect of guarding are applied separately, so the formula should not include them.
  - **Not stated:** whether the expression is evaluated as a script language, whether functions, comparisons or conditionals are allowed, rounding, a damage cap, and what happens with negative results.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-020 — Damage: Variance
- **What:** A random spread applied to the computed amount.
- **Behaviour:**
  - Ranges from 0 to 100%.
  - Worked example from the help: a base of 100 with 20% variance gives a result anywhere from 80 to 120.
  - The random distribution is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-021 — Damage: Critical Hits
- **What:** Whether the skill can land a critical hit.
- **Behaviour:**
  - Options are Yes or No.
  - With Yes, the critical chance comes from the user's Critical Chance and the target's Critical Evasion Rate.
  - The critical damage multiplier is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

### RM-SKL-022 — Effects
- **What:** Additional non-damage outcomes of the skill.
- **Behaviour:**
  - Set in the Effects window, opened by double-clicking the list.
  - Uses the shared effects editor (RM-DB-009).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-SKL-023 — Notes
- **What:** The developer's notes for the skill.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help". See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html

---

### RM-ITM-001 — Item data role
- **What:** Items are everything other than equipment. They can have effects that trigger when an actor uses them.
- **Behaviour:**
  - The item page has no Message field and no Required Weapon field. Those exist only on skills.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-002 — Name
- **What:** The item's display name.
- **Behaviour:**
  - A long name may be cut off on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-003 — Icon
- **What:** An image shown with the item name during play.
- **Behaviour:**
  - Double-clicking opens the "Select an Image" window.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-004 — Description
- **What:** Help text for the item.
- **Behaviour:**
  - Shown on the play screen when the item is selected.
  - No line limit is stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-005 — Item Type
- **What:** Controls where the item appears.
- **Behaviour:**
  - The item types are:
    - **Regular items:** the normal case. The exact label of this option is not verified.
    - **Key Item:** listed in a separate section of the in-game item menu, apart from regular items.
    - **Hidden Item A** and **Hidden Item B:** never shown on the item screen. They only appear when the player is asked to pick an item during an event.
  - How A differs from B is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-006 — Price
- **What:** The shop purchase price.
- **Behaviour:**
  - Items sell for half their purchase price.
  - A price of 0 means the item cannot be sold.
  - The maximum price is not stated.
  - How half of an odd price is rounded is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-007 — Consumable
- **What:** Whether using the item uses it up.
- **Behaviour:**
  - Options are Yes and No.
  - With Yes, the held quantity drops by one on each use.
  - The behaviour with No (item kept) is implied but not spelled out.
  - The default is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-008 — Occasion
- **What:** Where the item can be used.
- **Behaviour:**
  - Always: in battle and from the menu screen.
  - Battle Screen: in battle only.
  - Menu Screen: from the menu screen only.
  - Never: cannot be used anywhere.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-009 — Scope (Faction / Number / Status)
- **What:** Who the item targets. Same three-field system as skills; all three fields must be set.
- **Behaviour:**
  - **Faction:** Enemy, Ally, Enemy & Ally, User, or Unspecified.
  - **Number:** 1 Enemy/Ally, All Enemies/Allies, or Random.
  - **Status:** Alive, Knocked Out, or No Condition. It can only be set when targeting allies.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-010 — Speed
- **What:** An action-order modifier.
- **Behaviour:**
  - An integer from -2000 to 2000, added to the user's Agility when the item is used.
  - It affects turn order in battle.
  - Which sign acts earlier is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-011 — Success
- **What:** The item's base chance of success.
- **Behaviour:**
  - Ranges from 0 to 100%.
  - The final chance is also affected by the item's effectiveness on the target.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-012 — Repeat
- **What:** How many times the effect is applied per use.
- **Behaviour:**
  - Ranges from 1 to 9.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-013 — TP Gain
- **What:** TP the user gains when the item takes effect.
- **Behaviour:**
  - Granted after the effect lands on a target on a successful use.
  - The range is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-014 — Hit Type
- **What:** How hitting is decided. Same three options as skills.
- **Behaviour:**
  - **Certain Hit:** always hits once the use succeeds. Counterattack, Magic Reflection and Substitute have no effect on it.
  - **Physical Attack:** the user's Accuracy against the target's Evasion. Counterattack and Substitute can apply.
  - **Magic Attack:** the target's Magic Evasion. Magic Reflection and Substitute can apply.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-015 — Animation
- **What:** The battle animation played when the item is used.
- **Behaviour:**
  - No options are listed.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-016 — Damage: Type
- **What:** What the item's formula does to HP or MP.
- **Behaviour:**
  - One of six choices.
  - The help describes Damage (lowers HP or MP), Recover (raises it) and Drain (moves an amount from the target to the user).
  - The full names of all six options are not verified.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-017 — Damage: Element
- **What:** The element attached to the item's damage.
- **Behaviour:**
  - The choices are not listed.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-018 — Damage: Formula
- **What:** The same expression system as skills (RM-SKL-019).
- **Behaviour:**
  - a is the user and b the target, read with the dotted property form.
  - The readable properties are the same twelve as for skills: atk, def, mat, mdf, agi, luk, mhp, mmp, hp, mp, tp, level.
  - v followed by a number in brackets reads a game variable.
  - The four arithmetic operators are available.
  - The help gives the same example of four times the user's attack minus twice the target's defense.
  - Elements and guarding are applied separately, outside the formula.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-019 — Damage: Variance
- **What:** A random spread applied to the amount.
- **Behaviour:**
  - Ranges from 0 to 100%.
  - Same worked example as skills: 100 with 20% variance gives 80 to 120.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-020 — Damage: Critical Hits
- **What:** Whether the item can land a critical hit.
- **Behaviour:**
  - Options are Yes or No.
  - With Yes, the chance comes from the user's Critical Chance and the target's Critical Evasion Rate.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

### RM-ITM-021 — Effects
- **What:** Non-damage outcomes of using the item.
- **Behaviour:**
  - Set in the Effects window, opened by double-clicking the list.
  - Uses the shared effects editor (RM-DB-009).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_16.html

### RM-ITM-022 — Notes
- **What:** The developer's notes for the item.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help...". See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_04.html

---

### RM-WPN-001 — Weapon data role
- **What:** Weapons are actor equipment that can raise or lower values and grant special traits while equipped.
- **Behaviour:**
  - Weapons and armors share one help page. Fields marked weapon-only or armor-only below come from that page.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-002 — Name
- **What:** The weapon's display name.
- **Behaviour:**
  - A long name may be cut off on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-003 — Icon
- **What:** An image shown with the name during play.
- **Behaviour:**
  - Double-clicking opens the "Select an Image" window.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-004 — Description
- **What:** Help text for the weapon.
- **Behaviour:**
  - Shown when the weapon is selected on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-005 — Weapon Type
- **What:** The weapon's category.
- **Behaviour:**
  - Whether an actor can equip the weapon depends on the Equip Weapon traits of the actor or its class (see RM-DB-008).
  - The help says the list of weapon types can be changed via Terms. The type lists themselves appear to live on the Type tab, which is not verified in this slice.
  - The weapon type is also what a skill's Required Weapon checks (RM-SKL-016).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-006 — Price
- **What:** The shop purchase price.
- **Behaviour:**
  - Sells for half the purchase price.
  - A price of 0 means it cannot be sold.
  - The maximum is not stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-007 — Animation (weapon only)
- **What:** The animation played when the actor attacks with this weapon.
- **Behaviour:**
  - Only present on weapons.
  - No option list is given.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-008 — Parameter Changes
- **What:** Flat bonuses or penalties to the wearer's parameters while the weapon is equipped.
- **Behaviour:**
  - Max HP and Max MP range from -5000 to 5000.
  - Every other parameter ranges from -500 to 500.
  - Negative values lower the parameter.
  - The page does not list which parameters appear in the grid.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-WPN-009 — Traits
- **What:** Traits granted to the actor while the weapon is equipped.
- **Behaviour:**
  - Each trait is set up in a detail window opened by double-clicking a row.
  - Uses the shared traits editor (RM-DB-008).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-WPN-010 — Notes
- **What:** The developer's notes for the weapon.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help...". See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

---

### RM-ARM-001 — Armor data role
- **What:** Armors are actor equipment that can raise or lower values and grant special traits while equipped.
- **Behaviour:**
  - Armors share the weapon help page and differ only in the fields noted below: an Equipment Type field instead of Animation.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-002 — Name
- **What:** The armor's display name.
- **Behaviour:**
  - A long name may be cut off on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-003 — Icon
- **What:** An image shown with the name during play.
- **Behaviour:**
  - Double-clicking opens the "Select an Image" window.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-004 — Description
- **What:** Help text for the armor.
- **Behaviour:**
  - Shown when the armor is selected on the play screen.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-005 — Armor Type
- **What:** The armor's category.
- **Behaviour:**
  - Whether an actor can equip the armor depends on the Equip Armor traits of the actor or its class (see RM-DB-008).
  - The help says the list of armor types can be changed via Terms.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-006 — Equipment Type (armor only)
- **What:** The slot the armor goes into.
- **Behaviour:**
  - The help names four slots: Shield, Head, Body and Accessory.
  - An actor can equip compatible armor in each slot.
  - Only present on armors.
  - Whether more slots can be added is not stated on this page; Terms lets equipment-type names be edited (RM-DB-010).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-007 — Price
- **What:** The shop purchase price.
- **Behaviour:**
  - Sells for half the purchase price.
  - A price of 0 means it cannot be sold.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-008 — Parameter Changes
- **What:** Flat parameter bonuses or penalties while the armor is equipped.
- **Behaviour:**
  - Max HP and Max MP range from -5000 to 5000.
  - Other parameters range from -500 to 500.
  - Negative values lower the parameter.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

### RM-ARM-009 — Traits
- **What:** Traits granted while the armor is equipped.
- **Behaviour:**
  - Each trait is set up in a detail window opened by double-clicking a row.
  - Uses the shared traits editor (RM-DB-008).
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html ; https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html

### RM-ARM-010 — Notes
- **What:** The developer's notes for the armor.
- **Behaviour:**
  - Right-clicking the field offers "Plugin Help...". See RM-DB-007.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_08_05.html

---

## 3. Not verified

These points were looked for on the fetched pages and are not stated there. Do not implement them from this document without another official source.

### Database-wide
1. Whether Change Maximum asks for confirmation before deleting entries.
2. Which of F4 and F5 goes to the previous entry and which to the next.
3. Any OK, Cancel, Apply, save or undo behaviour of the Database window.
4. Whether Paste and Clear work on several selected entries.
5. Whether Ctrl selection is supported.
6. The Data ID display format.
7. Where skill, weapon and armor type lists are edited. The help points to Terms, but the fetched Terms page does not list them. They are presumably on the Type tab, which is outside this slice.
8. The target of the full-context link on each page.

### Actors
9. Limits on Name, Nickname and Profile length or lines.
10. How images are chosen (picker window) and image size requirements.
11. The actor battle graphic in front-view mode.
12. What happens when Initial Level is above Max Level.
13. Default values of all actor fields.

### Classes
14. The exact names of the eight parameter tabs. Max HP and Max MP are named; the rest are inferred from the Skill formula help.
15. Ranges, defaults and the combining formula of the EXP curve values (Base Value, Extra Value, Acceleration A, Acceleration B).
16. The level range of the EXP curve window.
17. The values and shapes of Quick Settings presets A to E.
18. The interpolation math of Generate Curve and the slider's numeric range.
19. Whether the parameter graph can be edited by dragging (the help only describes reading values).
20. Several skills per level, or what happens to skills from levels already passed.

### Skills
21. The meaning of Skill Type "None" (only implied to be unrestricted).
22. Which sign of Speed makes an action earlier.
23. How Success combines with the target's effectiveness.
24. The range of TP Gain, and whether it is granted per target or per repeat.
25. The full names of the six Damage Type options, and whether a "None" option exists.
26. The list of Element choices, for example "Normal Attack".
27. The list of Animation choices.
28. **Formula engine details:**
    - whether it is script-evaluated;
    - whether math functions, comparisons or conditionals are allowed;
    - rounding;
    - a damage cap;
    - how negative results are handled;
    - which additional properties beyond the twelve listed are available.
29. The variance distribution.
30. The critical hit multiplier.
31. Whether Required Weapon applies to enemies.
32. Special roles of skill IDs 1 and 2 (Attack and Guard).

### Items
33. The exact label of the regular Item Type option.
34. How Hidden Item A and Hidden Item B differ.
35. The Consumable default, and the behaviour when it is set to No.
36. The maximum price, and how half prices are rounded.
37. The TP Gain range.
38. The direction of Speed.
39. The six Damage Type names.
40. The Element choices.

### Weapons and armors
41. Which parameters appear in the Parameter Changes grid (names not listed).
42. The maximum price.
43. The options of the weapon Animation field.
44. Whether equipment slots beyond Shield, Head, Body and Accessory can be added.
45. Default values for all fields.
