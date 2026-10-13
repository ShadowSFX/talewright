# RPG Maker MZ: Event Commands, Part 2 (categories 9–16)

Clean-room feature inventory for Talewright. Everything below is paraphrased from the official RPG Maker MZ help (English). No sentences are copied; command and field names are kept as the help spells them. Nothing here comes from memory. When a page does not say something, the item says "not verified".

## Pages covered

| # | Category | URL | Load status |
|---|----------|-----|-------------|
| 9 | Timing | https://rpgmakerofficial.com/product/MZ_help-en/01_10_09.html | Loaded (HTTP 200) |
| 10 | Screen | https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html | Loaded (HTTP 200) |
| 11 | Audio & Video | https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html | Loaded (HTTP 200) |
| 12 | Scene Control | https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html | Loaded (HTTP 200) |
| 13 | System Settings | https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html | Loaded (HTTP 200) |
| 14 | Map | https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html | Loaded (HTTP 200) |
| 15 | Battle | https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html | Loaded (HTTP 200) |
| 16 | Advanced | https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html | Loaded (HTTP 200) |
| — | Event Commands index (to confirm the command list and tab layout) | https://rpgmakerofficial.com/product/MZ_help-en/01_10.html | Loaded (HTTP 200) |

How the pages were read:
- Each page was fetched with WebFetch. Its raw HTML was also downloaded so the full text and the dialog screenshots on the page could be checked line by line. Every screenshot on these 8 pages was viewed (42 images under `assets/img/01_10_09..16_img*.png`).
- Sub-links: apart from the breadcrumb links (`01.html`, `01_10.html`), these pages link to no other help pages. There were no in-help sub-links to follow.
- Tabs: the index page puts these categories in the event-command picker as follows. Tab 2 holds Timing, Screen, and Audio & Video. Tab 3 holds Scene Control, System Settings, Map, Battle, and Advanced.
- Screenshot values: values marked "screenshot" are what the dialog image on the page shows. The text never calls them defaults, so treat them as likely initial values, not confirmed defaults.
- Name differences between the help and the task brief:
  - "Replay BGM" (index) is "Resume BGM" (page heading).
  - "Change Encounter" (dialog title) is "Enable/Disable Encounters" (heading).
  - "Enemy Appear" (dialog title) is "Enemy Appearance" (heading).
  - "Flash" (heading) is "Flash Screen" (dialog title).
  - The audio field is "Balance" in the text but "Pan" in the screenshots.

---

## Timing

### RM-CMD2-001 — Timing: Wait
- **What:** Pauses the running event for a set number of frames.
- **Behavior:**
  - Parameter **Duration**: a frame count from 1 to 999. A frame is 1/60 s. The dialog's unit label reads "frames (1/60 sec)".
  - While the pause lasts, the player cannot act. The exception is processing that runs in parallel, which keeps going.
  - Screenshot: Duration shows 60.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_09.html#p01

---

## Screen

### RM-CMD2-002 — Screen: Fadeout Screen
- **What:** Fades the whole screen gradually to black.
- **Behavior:**
  - No parameters.
  - The screen stays black until a **Fadein Screen** command runs.
  - UI stays visible while the screen is faded out: menus, the message window, and the battle command window.
  - Used during battle, the whole battle view turns black (enemies too) while the command and message windows stay visible.
  - Fade speed/duration: not stated.
  - Map/battle restriction: none stated. The page describes its use in battle events.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p01

### RM-CMD2-003 — Screen: Fadein Screen
- **What:** Brings the screen back after a Fadeout Screen.
- **Behavior:**
  - No parameters.
  - Fade speed/duration: not stated.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p02

### RM-CMD2-004 — Screen: Tint Screen
- **What:** Shifts the colour tone of the whole screen over a set number of frames.
- **Behavior:**
  - **Color Tone**:
    - **Red**, **Green** and **Blue** are amounts added to each channel, from -255 to 255.
    - **Gray** is the strength of a grayscale filter, from 0 to 255. A higher value gives a stronger effect.
    - Each value has a slider and a number box, and a preview area shows the result.
  - Preset buttons fill in tone values: **Normal**, **Dark**, **Sepia**, **Sunset**, **Night**. **Normal** returns to the original (untinted) tone. The preset numbers are not stated.
  - **Duration**: frames from 1 to 999 (1 frame = 1/60 s) over which the tint changes.
  - **Wait for Completion** (checkbox): when on, the event waits until the tint change ends.
  - Pictures and message windows are not tinted.
  - The tint persists until this command changes it again, and it stays in effect during battle.
  - Screenshot: R/G/B/Gray at 0/0/0/0, Duration 60, Wait for Completion checked.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p03

### RM-CMD2-005 — Screen: Flash (dialog title "Flash Screen")
- **What:** Fills the whole screen with a colour for a moment, then fades it back to normal, for effects such as lightning.
- **Behavior:**
  - **Flash Color**:
    - **Red**, **Green** and **Blue** each run from 0 to 255.
    - **Intensity** is the colour's opacity, from 0 to 255. At 0 the flash is fully transparent and nothing visible happens.
    - A preview area shows the colour.
  - **Duration**: frames from 1 to 999 (1 frame = 1/60 s).
  - **Wait for Completion** (checkbox): when on, the event waits until the flash ends.
  - Screenshot: R/G/B 255/255/255, Intensity 170, Duration 60, Wait for Completion checked.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p04

### RM-CMD2-006 — Screen: Shake Screen
- **What:** Shakes the whole screen sideways (left and right).
- **Behavior:**
  - **Shake**: **Power** (1–9) sets how far the screen moves. **Speed** (1–9) sets how fast it oscillates.
  - **Duration**: frames from 1 to 999 (1 frame = 1/60 s).
  - **Wait for Completion** (checkbox): when on, the event waits until the shake ends.
  - Vertical shake: not mentioned (horizontal only).
  - Screenshot: Power 5, Speed 5, Duration 60, Wait for Completion checked.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p05

### RM-CMD2-007 — Screen: Set Weather Effect
- **What:** Turns on-screen weather on or off: rain, storm or snow.
- **Behavior:**
  - **Type**: which weather image to show. **None** clears the weather. The text names the three effects as rain, storm and snow.
  - **Power**: how many weather images are on screen (1–9).
  - **Duration**: frames from 1 to 999 (1 frame = 1/60 s) over which the weather changes.
  - **Wait for Completion** (checkbox): when on, the event waits until the change ends.
  - **Cannot be used in battle events.**
  - Screenshot: Type None, Power 5, Duration 60, Wait for Completion checked.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html#p06

---

## Audio & Video

### RM-CMD2-008 — Audio & Video: Play BGM
- **What:** Starts a background music (BGM) track.
- **Behavior:**
  - **File List**: the BGM file to play. Choosing **(None)** stops the music.
  - **Play/Stop** buttons: preview the chosen track in the editor with the current settings, or stop the preview.
  - **Volume**: playback volume. The text gives no range; the screenshot shows a percentage.
  - **Pitch**: 50%–150%. Above 100% the track plays faster and sounds higher.
  - **Balance**: -100 to 100. Negative values favor the left speaker and positive values favor the right. The screenshot labels this field "Pan".
  - If the chosen BGM is already playing, the command does nothing.
  - Screenshot: (None) highlighted, Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p01

### RM-CMD2-009 — Audio & Video: Fadeout BGM
- **What:** Lowers the BGM volume gradually, then stops it.
- **Behavior:**
  - **Duration**: length of the fade, in **seconds** (1–60), not frames.
  - Screenshot: Duration 10 seconds.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p02

### RM-CMD2-010 — Audio & Video: Save BGM
- **What:** Records the BGM now playing, including how far into the track it is.
- **Behavior:**
  - No parameters.
  - Stores the current track together with its playback position, to be restored by Resume BGM.
  - Number of save slots: not stated (presumably one; not verified).
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p03

### RM-CMD2-011 — Audio & Video: Resume BGM (listed as "Replay BGM" in the index)
- **What:** Plays again the BGM that Save BGM stored.
- **Behavior:**
  - No parameters.
  - Resumes the saved track. Since Save BGM keeps the playback position, resuming should continue from there. The page does not explicitly say it resumes from that position (see Not verified).
  - What happens with no saved BGM: not stated.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p04

### RM-CMD2-012 — Audio & Video: Play BGS
- **What:** Starts a background sound (BGS) loop.
- **Behavior:**
  - **File List**: the BGS file. **(None)** stops the background sound.
  - **Play/Stop** buttons: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%. Above 100% means faster and higher.
  - **Balance**: -100 to 100, from left to right. The screenshot labels it "Pan".
  - If the chosen BGS is already playing, the command does nothing.
  - Screenshot: Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p05

### RM-CMD2-013 — Audio & Video: Fadeout BGS
- **What:** Lowers the BGS volume gradually, then stops it.
- **Behavior:**
  - **Duration**: fade length in seconds (1–60).
  - Screenshot: 10 seconds.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p06

### RM-CMD2-014 — Audio & Video: Play ME
- **What:** Plays a music effect (ME), a short jingle.
- **Behavior:**
  - **File List**: the ME file. **(None)** stops the music.
  - **Play/Stop** buttons: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%.
  - **Balance**: -100 to 100. The screenshot labels it "Pan".
  - If the chosen ME is already playing, the command does nothing.
  - How an ME interacts with the BGM (for example, pausing it): not stated.
  - Screenshot: Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p07

### RM-CMD2-015 — Audio & Video: Play SE
- **What:** Plays a sound effect (SE).
- **Behavior:**
  - **File List**: the SE file.
  - **Play/Stop** buttons: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%.
  - **Balance**: -100 to 100. The screenshot labels it "Pan".
  - SEs can overlap: starting one while another is still playing plays both together.
  - Playing **(None)** does not stop SEs that are already playing. Use **Stop SE** for that.
  - Screenshot: the list includes (None). Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p08

### RM-CMD2-016 — Audio & Video: Stop SE
- **What:** Stops every sound effect that is playing.
- **Behavior:**
  - No parameters.
  - Affects all SEs at once.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p09

### RM-CMD2-017 — Audio & Video: Play Movie
- **What:** Plays a movie file.
- **Behavior:**
  - **File List**: the movie file. The screenshot lists a (None) entry.
  - The page gives no other settings: no volume, no skip option, no wait option.
  - Supported formats, whether the event waits for the movie to finish, and whether the player can skip: not stated.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html#p10

---

## Scene Control

### RM-CMD2-018 — Scene Control: Battle Processing
- **What:** Brings in a troop of enemies and starts a battle.
- **Behavior:**
  - **Troops**: three ways to pick the enemy troop:
    - **Direct Designation**: choose one troop from the list.
    - **Designation with a Variable**: the troop ID is read from a chosen variable.
    - **Same as Random Encounter(s)**: the troop is picked from the **Encounter** settings of the map the party is on.
  - **Can Escape** (checkbox):
    - Turns on the **Escape** command during the battle.
    - Adds branches after the command for **If Player Won** and **If Player Escaped**.
  - **Can Lose** (checkbox):
    - A party wipe no longer causes game over.
    - Adds branches for **If Player Won** and **If Player Lost**.
  - With both options on, the result branches are Won, Escaped and Lost. The page describes each option separately and does not show the combined layout (see Not verified).
  - With both options off: nothing here says escape is disabled, but enabling Escape is tied to the checkbox, which implies it is off by default. A party wipe then leads to game over.
  - Related: **Abort Battle** (RM-CMD2-050) runs the Escape branch when Can Escape is on.
  - Screenshot: Direct designation selected (troop "0001 Goblin*2"), both checkboxes off.
  - **Cannot be used in battle events.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p01

### RM-CMD2-019 — Scene Control: Shop Processing
- **What:** Starts a shop scene in which the party purchases goods (items, weapons, armor) from a configured list.
- **Behavior:**
  - **Merchandise**: the list of goods for sale, with two columns, Merchandise and Price.
    - Double-click an empty row to open an entry dialog where you set the good and its price.
    - Price mode **Standard** uses the price stored in the database.
    - Price mode **Specify** sets a custom price for this shop only.
  - **Purchase Only** (checkbox): when on, selling is turned off.
  - Implied by Purchase Only: the shop can also sell items when the checkbox is off.
  - Screenshot: empty merchandise list, Purchase Only off.
  - Maximum number of goods, quantity limits, and how sell prices are computed: not stated.
  - **Cannot be used in battle events.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p02

### RM-CMD2-020 — Scene Control: Name Input Processing
- **What:** Opens a name-entry screen for one actor, and what the player types becomes that actor's name.
- **Behavior:**
  - **Actor**: the actor to rename.
  - **Max characters**: name length limit, 1–16.
  - In game:
    - The D-pad moves the cursor.
    - The confirm button enters the selected character.
    - The delete button removes the last character.
  - Screenshot: Actor "0001 Reid", Max characters 8.
  - **Cannot be used in battle events.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p03

### RM-CMD2-021 — Scene Control: Open Menu Screen
- **What:** Opens the main menu screen.
- **Behavior:**
  - No parameters.
  - Whether it ignores the setting from Change Menu Access: not stated.
  - **Cannot be used in battle events.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p04

### RM-CMD2-022 — Scene Control: Open Save Screen
- **What:** Opens the save screen.
- **Behavior:**
  - No parameters.
  - Whether it ignores the setting from Change Save Access: not stated.
  - **Cannot be used in battle events.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p05

### RM-CMD2-023 — Scene Control: Game Over
- **What:** Ends the game on the spot and shows the Game Over screen.
- **Behavior:**
  - No parameters.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p06

### RM-CMD2-024 — Scene Control: Return to Title Screen
- **What:** Ends the game on the spot and goes back to the Title Screen.
- **Behavior:**
  - No parameters.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_12.html#p07

---

## System Settings

### RM-CMD2-025 — System Settings: Change Battle BGM
- **What:** Sets the music that plays in battle.
- **Behavior:**
  - **File List**: the BGM to use. **(None)** means no music plays.
  - **Play/Stop**: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%.
  - **Balance**: -100 to 100. The screenshot labels it "Pan".
  - The setting lasts until this command is run again.
  - If run during a battle, the change applies from the next battle.
  - If the map BGM and the battle BGM are the same track, the music keeps playing through the switch between map and battle.
  - Screenshot: "Battle1" selected, Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated. The page describes running it during battle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p01

### RM-CMD2-026 — System Settings: Change Victory ME
- **What:** Sets the ME that plays after a battle is won.
- **Behavior:**
  - **File List**: the ME. **(None)** means none plays.
  - **Play/Stop**: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%.
  - **Balance**: -100 to 100. The screenshot labels it "Pan".
  - The setting lasts until this command is run again.
  - Screenshot: "Victory1" selected, Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p02

### RM-CMD2-027 — System Settings: Change Defeat ME
- **What:** Sets the ME that plays after a battle is lost.
- **Behavior:**
  - **File List**: the ME. **(None)** means none plays.
  - **Play/Stop**: editor preview.
  - **Volume**: no range given in the text.
  - **Pitch**: 50%–150%.
  - **Balance**: -100 to 100. The screenshot labels it "Pan".
  - The setting lasts until this command is run again.
  - Screenshot: "Defeat1" selected, Volume 90%, Pitch 100%, Pan 0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p03

### RM-CMD2-028 — System Settings: Change Vehicle BGM
- **What:** Sets the music that plays while the party rides a given vehicle.
- **Behavior:**
  - **Vehicle**: which vehicle to change.
  - **BGM**: the track to use, chosen through a "..." picker.
  - Vehicle options listed: not stated. The screenshot shows "Boat", and the other choices (ship, airship) are not listed.
  - Screenshot: Vehicle "Boat", BGM "Ship1".
  - Whether the change persists: not stated, unlike its neighboring commands.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p04

### RM-CMD2-029 — System Settings: Change Save Access
- **What:** Allows or blocks saving by the player.
- **Behavior:**
  - **Save**: **Disable** blocks saving and **Enable** allows it.
  - The setting lasts until this command is run again.
  - Screenshot: Disable selected.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p05

### RM-CMD2-030 — System Settings: Change Menu Access
- **What:** Allows or blocks opening the Menu Screen by the player.
- **Behavior:**
  - **Menu**: **Disable** blocks the menu and **Enable** allows it.
  - The setting lasts until this command is run again.
  - Screenshot: Disable selected.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p06

### RM-CMD2-031 — System Settings: Enable/Disable Encounters (dialog title "Change Encounter")
- **What:** Turns random encounters (random battles against troops) on or off.
- **Behavior:**
  - **Encounter**: **Disable** stops encounters and **Enable** allows them.
  - The setting lasts until this command is run again.
  - Screenshot: Disable selected.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p07

### RM-CMD2-032 — System Settings: Change Formation Access
- **What:** Allows or blocks the player from rearranging the party order (formation).
- **Behavior:**
  - **Formation**: **Disable** blocks it and **Enable** allows it.
  - The setting lasts until this command is run again.
  - Screenshot: Disable selected.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p08

### RM-CMD2-033 — System Settings: Change Window Color
- **What:** Changes the colour tone applied to game windows.
- **Behavior:**
  - **Window Color**: **Red**, **Green** and **Blue** values, each from -255 to 255. Each has a slider and a number box, and a preview swatch shows the result.
  - The setting lasts until this command is run again.
  - Screenshot: R/G/B 0/0/0.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p09

### RM-CMD2-034 — System Settings: Change Actor Images
- **What:** Swaps the graphics used for one actor.
- **Behavior:**
  - **Actor**: the actor to change.
  - **Images**: three slots, each opened by double-clicking its box:
    - **Face**
    - **Walking Character** (labeled "Character" in the dialog)
    - **[SV] Battler**
  - Choosing **(None)** in a slot means no image is shown for it.
  - The setting lasts until this command is run again.
  - Screenshot: Actor "0001 Reid", with all three images filled in.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p10

### RM-CMD2-035 — System Settings: Change Vehicle Image
- **What:** Swaps the graphic used for a vehicle.
- **Behavior:**
  - **Vehicle**: the vehicle to change.
  - **Images**: double-click the box to pick the graphic. **(None)** means no image is shown.
  - The setting lasts until this command is run again.
  - Screenshot: Vehicle "Boat", with a boat sprite.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_13.html#p11

---

## Map

### RM-CMD2-036 — Map: Change Map Name Display
- **What:** Controls whether a map's name appears on screen when the player arrives on a different map.
- **Behavior:**
  - **Map Name Display**: **ON** shows the name and **OFF** hides it.
  - Screenshot: ON selected.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html#p01

### RM-CMD2-037 — Map: Change Tileset
- **What:** Switches the tileset of the current map.
- **Behavior:**
  - **Tilesets**: the new tileset, picked from a dropdown.
  - Applies only to the map that contains the event.
  - Screenshot: "0001 Overworld".
  - Whether the change survives leaving and re-entering the map: not stated.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html#p02

### RM-CMD2-038 — Map: Change Battle Background
- **What:** Changes the battle background used on the current map.
- **Behavior:**
  - **Battle Background**: the "..." button opens a picker for two images, called left and right. A preview area shows the result.
  - Applies only to the map that contains the event.
  - If run inside a battle event, the new background applies from the next battle.
  - Map/battle restriction: none stated. The page covers running it in battle.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html#p03

### RM-CMD2-039 — Map: Change Parallax
- **What:** Changes the parallax background of the current map.
- **Behavior:**
  - **Images**: the new parallax image, chosen with a "..." picker.
  - **Loop Horizontally** (checkbox): tiles the image horizontally. Its **Scroll** box sets an auto-scroll speed from -32 to 32, and 0 means no scrolling.
  - **Loop Vertically** (checkbox): the same for the vertical axis, with its own **Scroll** box (-32 to 32, 0 means no scrolling).
  - Applies only to the map that contains the event.
  - Screenshot: Image empty, both loops off, Scroll boxes grayed out while their loop is off.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html#p04

### RM-CMD2-040 — Map: Get Location Info
- **What:** Reads a value from a map location and stores it in a variable.
- **Behavior:**
  - **Variable**: where the result goes.
  - **Info Type**: what to read. The text does not list the choices. The screenshot shows "Terrain Tag" as one option.
  - **Location**: three modes:
    - **Direct designation**: pick a tile with the "..." picker. The screenshot shows "Current Map (7,6)".
    - **Designation with variables**: X and Y are read from two chosen variables. The text mentions a map ID and coordinates for this mode, but the screenshot shows only X and Y boxes.
    - **Character** (dialog: "Designation by a character"): uses the tile a chosen character is standing on, picked from a dropdown.
  - Only locations on the map that contains the event can be read.
  - Screenshot: Variable 0001, Info Type Terrain Tag, Direct designation.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_14.html#p05

---

## Battle

### RM-CMD2-041 — Battle: Change Enemy HP
- **What:** Adds to or subtracts from an enemy's HP.
- **Behavior:**
  - **Enemies**: one enemy, or **Entire Troop** for all enemies.
  - **Controls** (dialog: "Operation"): **Increase** or **Decrease**.
  - **Operand**: **Constant** (a number you type) or **Variable** (the amount is read from a variable).
  - **Allow Death** (checkbox):
    - On: an enemy whose HP reaches 0 or below is knocked out.
    - Off: HP stops at 1 instead of dropping to 0 or below.
  - Screenshot: Entire Troop, Increase, Constant 1. Allow Death is shown grayed out (disabled) while Increase is selected. The text does not explain this.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p01

### RM-CMD2-042 — Battle: Change Enemy MP
- **What:** Adds to or subtracts from an enemy's MP.
- **Behavior:**
  - **Enemies**: one enemy, or **Entire Troop** for every enemy currently in the battle.
  - **Controls** ("Operation" in the dialog): **Increase** or **Decrease**.
  - **Operand**: **Constant** or **Variable**.
  - Screenshot: Entire Troop, Increase, Constant 1.
  - Clamping at 0 or at max MP: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p02

### RM-CMD2-043 — Battle: Change Enemy TP
- **What:** Adds to or subtracts from an enemy's TP.
- **Behavior:**
  - **Enemies**: one enemy, or **Entire Troop** for every enemy currently in the battle.
  - **Controls** ("Operation" in the dialog): **Increase** or **Decrease**.
  - **Operand**: **Constant** or **Variable**.
  - Screenshot: Entire Troop, Increase, Constant 1.
  - TP limits: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p03

### RM-CMD2-044 — Battle: Change Enemy State
- **What:** Adds a state to an enemy or removes one.
- **Behavior:**
  - **Enemies**: one enemy, or **Entire Troop** for every enemy currently in the battle.
  - **Controls** ("Operation" in the dialog): **Add** or **Remove**.
  - **States**: which state to apply or clear.
  - Screenshot: Entire Troop, Add, State "0001 Dead".
  - Whether state resistance can block the change: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p04

### RM-CMD2-045 — Battle: Enemy Recover All
- **What:** Restores an enemy to full HP and MP and clears all of its states.
- **Behavior:**
  - **Enemies**: one enemy, or **Entire Troop** for every enemy currently in the battle.
  - Whether it revives a knocked-out enemy: not explicitly stated. It clears all states, and the screenshot of Change Enemy State shows "Dead" as a state, but the page does not draw that conclusion.
  - Screenshot: Entire Troop.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p05

### RM-CMD2-046 — Battle: Enemy Appearance (dialog title "Enemy Appear")
- **What:** Brings into the battle an enemy that the Troop setup marks with **Appear Mid-Battle**.
- **Behavior:**
  - **Enemies**: the enemy to reveal. The dropdown lists troop slots by number, and the screenshot shows "#1 ?".
  - It applies to enemies flagged with **Appear Mid-Battle** in the Troop setup. What happens if you pick an enemy without that flag is not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p06

### RM-CMD2-047 — Battle: Enemy Transform
- **What:** Turns an enemy in the battle into a different enemy type.
- **Behavior:**
  - **Enemies**: the enemy to transform, chosen by troop slot. The screenshot shows "#1 ?".
  - **Transform To**: the enemy it becomes. The screenshot shows "0001 Goblin".
  - The enemy's HP and MP from before the change carry over to the new form.
  - What happens to states, TP and buffs: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p07

### RM-CMD2-048 — Battle: Show Battle Animation
- **What:** Plays an animation with an enemy as its target.
- **Behavior:**
  - **Enemies**: the target enemy. The screenshot shows **Entire Troop** as a choice in this dropdown.
  - **Animations**: the animation to play, picked with "...". The screenshot shows "0001 Hit Physical".
  - Whether actors can be targeted: not stated. The text mentions enemies only.
  - Whether the event waits for the animation to finish: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p08

### RM-CMD2-049 — Battle: Force Action
- **What:** Makes an enemy or an actor use a chosen skill.
- **Behavior:**
  - **Subject**: radio choice of **Enemy** (by troop slot) or **Actor** (from a dropdown).
  - **Action**:
    - **Skill**: the skill to use, picked with "...".
    - **Target** choices:
      - **Last Target**: the same target as the subject's previous action.
      - **Random**: a randomly chosen target.
      - **Index X**: X is 1–8.
  - The page says to see the remarks for more on Index X, but the Remarks section has no such explanation (see Not verified).
  - Timing (whether the action happens at once or on the next turn) and whether the forced action uses up the subject's turn: not stated.
  - Screenshot: Enemy "#1 ?", Skill "0001 Attack", Target "Last Target".
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p09

### RM-CMD2-050 — Battle: Abort Battle
- **What:** Ends the current battle immediately and sends the game back to the map.
- **Behavior:**
  - No parameters.
  - If the battle came from **Battle Processing** with **Can Escape** on, the commands in that command's **Escape** branch are run.
  - An aborted battle is not added to the game's escape count, the counter that Control Variables can read under "Others".
  - Win/lose branches after an abort, and whether an abort with Can Escape off leads to game over: not stated.
  - **Battle events only.**
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_15.html#p10

---

## Advanced

### RM-CMD2-051 — Advanced: Script
- **What:** Runs the entered text as JavaScript.
- **Behavior:**
  - **Script**: a multi-line text area for the JavaScript to run.
  - Right-clicking the text area opens a context menu that includes **Plugin Help**, which shows plugin help information.
  - Length limits, what the code can access, and whether a return value is used: not stated.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html#p01

### RM-CMD2-052 — Advanced: Plugin Command
- **What:** Sends a command defined by a plugin to that plugin.
- **Behavior:**
  - **Plugin Command**:
    - **Plugin Name**: one of the plugins enabled in the Plugin Manager, chosen with a "..." picker.
    - **Command Name**: one of the commands that plugin defines, from a dropdown.
    - The screenshot also shows an empty panel under these two fields. It probably holds a description of the command; this is not verified.
  - **Arguments**: a Name/Value table for the arguments the plugin declares for that command.
  - The page lists no other commands under Advanced.
  - Map/battle restriction: none stated.
- **Source:** https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html#p02

---

## Not verified

These points were looked for on the pages above and are not stated there:

1. **Wait**: whether parallel-process events also stop when they run Wait themselves. The page only says parallel processes keep running during a pause.
2. **Fadeout / Fadein Screen**: fade duration or speed.
3. **Tint Screen**: the exact R/G/B/Gray values of the Normal, Dark, Sepia, Sunset and Night presets.
4. **Shake Screen**: any vertical shake option (the page describes horizontal only). The units behind Power and Speed.
5. **Set Weather Effect**: the dropdown values beyond "None". The text names rain, storm and snow, but the screenshot shows only "None".
6. **Audio (Play BGM/BGS/ME/SE, System Settings audio commands)**: the Volume range. The text gives none; the screenshot shows a percentage.
7. **Save BGM / Resume BGM**: the number of save slots. Whether Resume BGM explicitly continues from the stored position (the position is saved, but the resume wording does not restate it). What Resume does when nothing is saved.
8. **Play ME**: whether the BGM pauses or ducks while an ME plays.
9. **Play Movie**: supported file formats, whether the event waits for the movie to end, whether it can be skipped, and volume.
10. **Battle Processing**:
    - The exact branch layout when both Can Escape and Can Lose are on.
    - Default states of the options (the screenshot shows both unchecked).
    - Behavior of Same as Random Encounter on a map with no encounters set.
11. **Shop Processing**: the sell-price formula, the maximum number of goods, and quantity limits.
12. **Name Input Processing**: the character set or grid layout, and the starting text (the current name or blank).
13. **Open Menu Screen / Open Save Screen**: whether they work while Change Menu Access or Change Save Access is set to Disable.
14. **Game Over / Return to Title Screen**: whether they are usable in battle events. No restriction is stated for either.
15. **Change Vehicle BGM**: the list of vehicle options (only "Boat" is visible), and whether the setting persists. The other System Settings commands state that theirs does; this one does not.
16. **Change Tileset / Change Battle Background / Change Parallax**: whether the change persists after leaving and returning to the map.
17. **Get Location Info**:
    - The full list of **Info Type** options (only "Terrain Tag" is visible in the screenshot).
    - Whether "Designation with variables" also takes a map ID: the text mentions one, but the screenshot shows only X and Y.
    - The value stored when a location is invalid.
18. **Change Enemy HP**: why Allow Death appears grayed out while Increase is selected (presumably it only applies to Decrease).
19. **Change Enemy MP / TP**: clamping behavior.
20. **Change Enemy State**: interaction with state resistance.
21. **Enemy Recover All**: whether it revives a knocked-out enemy.
22. **Enemy Transform**: what happens to states, TP and buffs.
23. **Show Battle Animation**: whether actors can be targeted, and whether the event waits for the animation.
24. **Force Action**:
    - The meaning of **Index X**. The page points to its remarks, but the remarks contain no explanation.
    - When the forced action happens.
    - Whether it uses up the subject's turn.
25. **Abort Battle**: the outcome when the battle was started without Can Escape. Whether Won or Lost branches run.
26. **Script**: execution context, size limits, and how errors are handled.
27. **Plugin Command**: what the empty panel under Plugin Name / Command Name shows (probably a description). The argument types supported.
28. **Map/battle availability**: stated only where marked in bold above. Commands with no stated restriction are not confirmed as usable everywhere. The page simply does not say.
