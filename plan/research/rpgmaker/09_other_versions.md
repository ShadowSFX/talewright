# 09 — Other RPG Maker Versions vs. MZ (Talewright feature inventory)

Baseline: RPG Maker MZ. This file lists notable features of other RPG Maker versions that MZ lacks or handles differently. Every item cites an official page. All descriptions are paraphrased; no marketing text is copied. Research date: 2026-10-13.

"Relevance to Talewright" says whether MZ has the feature, checked against the official MZ help (https://rpgmakerofficial.com/product/MZ_help-en/), with the specific help page cited. The values are **yes**, **no**, **partial**, **n/a** or **not verified**.

---

## 1. Sources used

### 1.1 Official product, store and news pages

| # | URL | Status |
|---|-----|--------|
| S1 | https://rpgmakerofficial.com/en/news/4181/ | Loaded. Announcement headline only; links to the press-release PDF (S2). |
| S2 | https://www.gotchagotcha.jp/assets/pdf/20260521_RPGMAKERU2U.pdf | WebFetch failed (file over 10 MB). Downloaded with curl and text-extracted successfully. This is Gotcha Gotcha Games' own press release, dated 2026-05-21. |
| S3 | https://rpgmakerofficial.com/en/news/4265/ | Loaded. Announces that the Steam page is live; links to S4. |
| S4 | https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/ | Loaded |
| S5 | https://u2u.rpgmakerofficial.com/en | Loaded (landing page; little detail) |
| S6 | https://u2u.rpgmakerofficial.com/en/feature/ | Loaded |
| S7 | https://u2u.rpgmakerofficial.com/en/story/ | Loaded, but it only lists headings. The article bodies (Producer Letters) are on x.com and were not fetched. |
| S8 | https://rpgmakerofficial.com/en/news/4155/ | Loaded. 2026-03 teaser for an unnamed "new project"; it carries no feature data. |
| S9 | https://www.rpgmakerweb.com/products | Loaded |
| S10 | https://www.rpgmakerweb.com/products/rpg-maker-mz | Loaded |
| S11 | https://www.rpgmakerweb.com/products/rpg-maker-mv | Loaded. Some section bodies on this page are copy-paste errors; only the coherent sections were used. |
| S12 | https://www.rpgmakerweb.com/products/rpg-maker-vx-ace | Loaded |
| S13 | https://www.rpgmakerweb.com/products/rpg-maker-vx | Loaded |
| S14 | https://www.rpgmakerweb.com/products/rpg-maker-xp | Loaded |
| S15 | https://www.rpgmakerweb.com/products/rpg-maker-2003 | Loaded |
| — | https://www.rpgmakerweb.com/products/rpg-maker-2000 | **FAILED (HTTP 404)** |
| — | https://www.rpgmakerweb.com/products/rpg-maker-unite | **FAILED (HTTP 404)** |
| — | https://www.rpgmakerweb.com/products/rpg-maker-with | **FAILED (HTTP 404)** |
| S16 | https://rpgmakerofficial.com/en/products/ | Loaded (product link list) |
| — | https://rpgmakerofficial.com/en/contents/history/ | Loaded, but the product list renders client-side, so no data was obtained. Counted as failed for content. |
| — | https://rpgmakerofficial.com/product/products/rpg2003/index/ | **FAILED (HTTP 404)**. There is no Japanese 2003 product page. |
| S17 | https://rpgmakerofficial.com/product/products/rpg2000/index/ | Loaded (JP; VALUE!+ re-release page) |
| S18 | https://rpgmakerofficial.com/product/products/rpgxp/index/ | Loaded (JP) |
| S19 | https://rpgmakerofficial.com/product/products/rpgxp/tokucho/index.html | Loaded (JP) |
| S20 | https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html | Loaded (JP) |
| S21 | https://rpgmakerofficial.com/product/products/rpgxp/battlesys/index.html | Loaded (JP) |
| S22 | https://rpgmakerofficial.com/product/products/rpgvx/index/ | Loaded (JP) |
| S23 | https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html | Loaded (JP) |
| S24 | https://rpgmakerofficial.com/product/products/rpgvxace/index/ | Loaded (JP) |
| S25 | https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html | Loaded (JP) |
| S26 | https://rpgmakerofficial.com/product/products/rpgvxace/outline/index.html | Loaded (JP) |
| S27 | https://rpgmakerofficial.com/product/mv/ | Loaded (JP; update news only, no feature data) |
| S28 | https://store.steampowered.com/app/383730/ (RPG Maker 2000) | Loaded |
| S29 | https://store.steampowered.com/app/362870/ (RPG Maker 2003) | Loaded |
| S30 | https://store.steampowered.com/app/235900/ (RPG Maker XP) | Loaded |
| S31 | https://store.steampowered.com/app/521880/ (RPG Maker VX) | Loaded |
| S32 | https://store.steampowered.com/app/220700/ (RPG Maker VX Ace) | Loaded |
| S33 | https://store.steampowered.com/app/363890/ (RPG Maker MV) | Loaded |
| S34 | https://store.steampowered.com/app/1650950/ (RPG Maker Unite) | Loaded (pre-release "Coming soon" listing) |
| — | https://store.steampowered.com/news/app/362870 and https://store.steampowered.com/news/app/4856440 | Loaded, but the news content renders client-side, so no data was obtained |
| S35 | https://store.rpgmakerofficial.com/products/rpg-maker-unite | Loaded |
| S36 | https://store.rpgmakerofficial.com/products/rpg-maker-2003 | Loaded |
| S37 | https://store.rpgmakerofficial.com/products/rpg-maker-xp | Loaded |
| S38 | https://rpgmakerunite.com/en/ | Loaded |
| S39 | https://rpgmakerunite.com/en/about/ | Loaded |
| S40 | https://rpgmakerunite.com/en/manuals/ | Loaded (table of contents) |
| S41 | https://rpgmakerunite.com/en/article/6932492229657/ ("What's New in RPG Maker UNITE") | Loaded |
| S42 | https://rpgmakerunite.com/en/article/13937815423513/ (Outline) | Loaded |
| S43 | https://rpgmakerunite.com/en/article/13572542408729/ (Add-on Manager) | Loaded |
| S44 | https://rpgmakerunite.com/en/article/35475324025497/ (Call Unity Scene) | Loaded |
| S45 | https://rpgmakerunite.com/en/article/35461927757977/ (Post Effects) | Loaded |
| S46 | https://rpgmakerunite.com/en/article/14950935540377/ (Map Settings) | Loaded |
| S47 | https://rpgmakerunite.com/en/article/9505520588185/ (UI Settings) | Loaded |
| S48 | https://rpgmakerunite.com/en/article/9517190933401/ (Edit Battles) | Loaded |
| S49 | https://rpgmakerunite.com/en/article/17905658031129/ (Launch date finalized) | Loaded |
| S50 | https://rpgmakerunite.com/en/article/42285977040281/ (WebGL build) | Loaded |
| S51 | https://rpgmakerofficial.com/en/news/1279/ (Unite launched on Asset Store) | Loaded |
| S52 | https://with.rpgmakerofficial.com/ | Loaded (JP) |
| S53 | https://with.rpgmakerofficial.com/product/ | Loaded (JP) |
| S54 | https://with.rpgmakerofficial.com/news/ | Loaded (JP) |
| — | https://with.rpgmakerofficial.com/en/ | **FAILED (HTTP 404)**. No English official WITH page was found. |

### 1.2 Official help manuals (used to check MZ, plus the MV help for MV-vs-predecessor facts)

| # | URL | Status |
|---|-----|--------|
| H0 | https://rpgmakerofficial.com/product/MZ_help-en/ | WebFetch returned only the client-side shell. The individual pages below were fetched directly with curl. |
| H1 | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_01.html (Main Features) | Loaded |
| H2 | https://rpgmakerofficial.com/product/MZ_help-en/01_01_01_03.html (Time Progression Battles) | Loaded |
| H3 | https://rpgmakerofficial.com/product/MZ_help-en/01_01_02.html (System Requirements) | Loaded |
| H4 | https://rpgmakerofficial.com/product/MZ_help-en/01_05.html (Aid Tools) | Loaded |
| H5 | https://rpgmakerofficial.com/product/MZ_help-en/01_07_01.html (Editing Map Designs) | Loaded |
| H6 | https://rpgmakerofficial.com/product/MZ_help-en/01_07_02.html (Map Data Controls) | Loaded |
| H7 | https://rpgmakerofficial.com/product/MZ_help-en/01_07_03.html (Map Properties) | Loaded |
| H8 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_02.html (Class Settings) | Loaded |
| H9 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_03.html (Skill Settings) | Loaded |
| H10 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_07.html (Troop Settings) | Loaded |
| H11 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_10.html (Tileset Settings) | Loaded |
| H12 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_01.html (System 1) | Loaded |
| H13 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_12_02.html (System 2) | Loaded |
| H14 | https://rpgmakerofficial.com/product/MZ_help-en/01_08_15.html (Traits) | Loaded |
| H15 | https://rpgmakerofficial.com/product/MZ_help-en/01_09_02.html (Map Event Creation / Quick Events) | Loaded |
| H16 | https://rpgmakerofficial.com/product/MZ_help-en/01_09_03.html (Map Event Settings) | Loaded |
| H17 | https://rpgmakerofficial.com/product/MZ_help-en/01_10_01.html (Messages) | Loaded |
| H18 | https://rpgmakerofficial.com/product/MZ_help-en/01_10_10.html (Screen) | Loaded |
| H19 | https://rpgmakerofficial.com/product/MZ_help-en/01_10_11.html (Audio & Video) | Loaded |
| H20 | https://rpgmakerofficial.com/product/MZ_help-en/01_10_16.html (Advanced: Script / Plugin Command) | Loaded |
| H21 | https://rpgmakerofficial.com/product/MZ_help-en/01_11_01.html (Asset Standards) | Loaded |
| H22 | https://rpgmakerofficial.com/product/MZ_help-en/01_11_03.html (Output Formats) | Loaded |
| H23 | https://rpgmakerofficial.com/product/MZ_help-en/01_11_04.html (Enabling Plugins) | Loaded |
| M1 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_01.html (MV: Format Support) | Loaded |
| M2 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_02.html (MV: Maps) | Loaded |
| M3 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_03.html (MV: Plugins) | Loaded |
| M4 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_04.html (MV: Database) | Loaded |
| M5 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_05.html (MV: Side-view Battle) | Loaded |
| M6 | https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_08.html (MV: Game) | Loaded |
| M7 | https://rpgmakerofficial.com/product/MV_Help/page/01_11_04.html (MV: Output Formats) | Loaded |
| M8 | https://rpgmakerofficial.com/product/MV_Help/page/01_11_05.html and https://rpgmakerofficial.com/product/MV_Help/page/01_11_06.html (MV: Android/iOS conversion) | Loaded |
| M9 | https://rpgmakerofficial.com/product/MV_Help/page/01_11_14.html (MV: Linux) | Loaded |

### 1.3 Non-official pages seen but NOT used as evidence
- The Steam Community "RPG Maker 2003 Changelog" thread (https://steamcommunity.com/app/362870/discussions/0/618460171314070796/) is a user post with no developer badge, so it does not count as official. It is listed only as a lead under "Not verified".
- The rpgmakerweb forums, rpgmaker.net, EasyRPG and news sites (Gematsu, RPG Site and others) were excluded.

---

## 2. Release overview (official dates only)

| Version | Official date(s) found | Source |
|---|---|---|
| RPG Maker 2000 | The English Steam release was 13 Jul 2015, and its text says the engine debuted about 15 years earlier. The JP official page sells the VALUE!+ edition (27 Sep 2012) with copyright years starting at 2000. | S28, S17 |
| RPG Maker 2003 | English Steam release 24 Apr 2015. The original JP release year is **not verified**. | S29 |
| RPG Maker XP | Steam lists 16 Sep 2005. The JP page sells the VALUE!+ edition (27 Sep 2012) with copyright years starting at 2004. | S30, S18 |
| RPG Maker VX | JP release 27 Dec 2007; Steam lists 29 Feb 2008 | S22, S31 |
| RPG Maker VX Ace | JP release 15 Dec 2011; Steam lists 10 Dec 2012 | S24, S32 |
| RPG Maker MV | Steam lists 23 Oct 2015 | S33 |
| RPG Maker MZ (baseline) | Summer 2020 | S10 |
| RPG Maker Unite | Unity Asset Store, 2023. The official sources conflict on the day: S49 says 27 Apr 2023, while S51 says it was released 8 May 2023. Epic Games Store 29 Feb 2024; Humble 19 Apr 2024; the Steam release was postponed and has no date. | S49, S51, S38 |
| RPG Maker WITH | Nintendo Switch (JP) 11 Apr 2024; PS5/PS4 (JP) 30 Jan 2025 | S53 |
| RPG Maker U2U | Announced 21 May 2026 (press release); EN news 15 Jun 2026; Steam page live 27 Aug 2026; release date "Coming soon", not set | S2, S1, S3, S4 |

---

## 3. Feature items

### RPG Maker 2000

### RM-OTH-001 — RPG Maker 2000: Front-view battles driven by conditions and battle events
- **What:** Combat uses a classic front-view, first-person layout. The official listing says battles can be made more dynamic by mastering in-battle conditions and events.
- **Relevance to Talewright:** yes. MZ offers a front-view battle mode (H12) and per-troop battle events with conditions and span (H10).
- **Source:** https://store.steampowered.com/app/383730/

### RM-OTH-002 — RPG Maker 2000: Database-centred, point-and-click workflow
- **What:** Characters, enemies, skills, animations and equipment all live in one database. Maps are painted tile by tile, and events are built from a menu of dozens of commands (messages, movement, choices, effects). The JP page describes the flow as map, then cast, then events.
- **Relevance to Talewright:** yes. MZ uses the same database, map and event triad (H5, H10, H16).
- **Source:** https://store.steampowered.com/app/383730/ ; https://rpgmakerofficial.com/product/products/rpg2000/index/

### RM-OTH-003 — RPG Maker 2000: MIDI and WAV audio import
- **What:** The engine ships with MIDI music and sound effects, and users can add their own WAV and MIDI files.
- **Relevance to Talewright:** no. MZ audio must be Ogg Vorbis, with optional LOOPSTART/LOOPLENGTH metadata (H21).
- **Source:** https://store.steampowered.com/app/383730/

### RM-OTH-004 — RPG Maker 2000: Retro 16-bit-style custom graphics
- **What:** Users can import their own sprites, tilesets, battle animations and backgrounds in a retro 16-bit console style.
- **Relevance to Talewright:** partial. MZ imports custom PNG assets, but on its own standard of 48x48 tiles and 3-pattern character sheets, with no retro mode (H21).
- **Source:** https://store.steampowered.com/app/383730/

### RM-OTH-005 — RPG Maker 2000: Single portable Windows export and commercial sale
- **What:** A finished game can be exported as a portable file that runs on Windows. The listing says games may be sold once the creator holds the rights and completes registration.
- **Relevance to Talewright:** partial. MZ deploys a game folder for Windows, macOS or the web rather than a single file (H22).
- **Source:** https://store.steampowered.com/app/383730/

### RPG Maker 2003

### RM-OTH-006 — RPG Maker 2003: First side-view battle system in the series
- **What:** RPG Maker 2003 introduced a third-person, side-view battle screen in the style of mid-90s console RPGs, set up through the database and playtesting with no scripting.
- **Relevance to Talewright:** yes. MZ has a side-view battle option (H12). Note that 2003's timing model (ATB) is **not verified** by an official source; see the Not verified list.
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-2003 ; https://store.steampowered.com/app/362870/

### RM-OTH-007 — RPG Maker 2003: Event Search tool
- **What:** 2003 had a tool that finds every place a given switch or variable is used. The official MV page calls MV's Event Searcher the return of this 2003 function.
- **Relevance to Talewright:** yes. MZ includes an Event Searcher among its aid tools (H4).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-mv

### RM-OTH-008 — RPG Maker 2003: MP3, WAV and MIDI audio import
- **What:** The bundled audio is MIDI music plus sound effects, and users can add MP3, WAV or MIDI files.
- **Relevance to Talewright:** no. MZ accepts only Ogg Vorbis audio (H21).
- **Source:** https://store.steampowered.com/app/362870/ ; https://store.rpgmakerofficial.com/products/rpg-maker-2003

### RM-OTH-009 — RPG Maker 2003: Sprite-sheet battle animations stored in the database
- **What:** Battle animations can be custom 16-bit graphics, and the animations are kept as database entries.
- **Relevance to Talewright:** partial. MZ keeps animations in the database but overhauled the system around Effekseer particle effects, which the official MZ page describes as working without sprite sheets (H1, S10).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-2003

### RPG Maker XP

### RM-OTH-010 — RPG Maker XP: RGSS, Ruby scripting of the core engine
- **What:** XP's built-in systems (character parameters, battle, menus) are written as editable Ruby scripts (RGSS, later called RGSS1). Editing them can change any function, rebuild menus or the battle system, and add new event commands. Community scripts can be pasted in.
- **Relevance to Talewright:** partial. MZ extends the engine through JavaScript plugins in a Plugin Manager plus a "Script" event command (H23, H20). It has no in-editor script editor exposing the core; the MV help says the traditional Script Editor was removed in MV (M3).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-xp ; https://store.steampowered.com/app/235900/ ; https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html

### RM-OTH-011 — RPG Maker XP: Three explicit map layers
- **What:** XP raised the number of map layers from two (earlier titles) to three, allowing more complex layered maps. The official MZ page calls MZ's layer selection the return of an XP feature.
- **Relevance to Talewright:** partial. MZ lets you choose Layer 1–4 or "Auto" (H5). Per-tile priority, the other half of XP's layering, is **not verified**; see the Not verified list.
- **Source:** https://store.steampowered.com/app/235900/ ; https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-mz

### RM-OTH-012 — RPG Maker XP: 640x480 game screen
- **What:** The game screen is 640x480, which the JP page says is four times the area of the original RPG Maker.
- **Relevance to Talewright:** yes. MZ lets the screen width and height be set in System 2 (H13).
- **Source:** https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-xp

### RM-OTH-013 — RPG Maker XP: Full-colour PNG with alpha, translucency and a chosen transparent colour
- **What:** XP supports full-colour PNG with alpha channels, adjustable image translucency, and designating a colour to treat as transparent.
- **Relevance to Talewright:** partial. MZ requires PNG images (H21). A user-designated transparent colour key is not described in the MZ help pages checked.
- **Source:** https://store.steampowered.com/app/235900/ ; https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html

### RM-OTH-014 — RPG Maker XP: Encrypted single-archive packaging and "game disc" output
- **What:** XP can encrypt a finished game by merging the data files and asset files, previously stored separately, into one archive so others cannot inspect them. Games can be saved as a game disc for CD-ROM or online distribution, or sold as shareware.
- **Relevance to Talewright:** partial. MZ encrypts image and audio files individually with a key and deploys a folder. There is no single merged archive (H22).
- **Source:** https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html ; https://rpgmakerofficial.com/product/products/rpgxp/tokucho/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-xp

### RM-OTH-015 — RPG Maker XP: Front-view battle with party portraits and animations on heroes
- **What:** XP battles are turn-based and front-view, but the party's battler graphics appear in the status area and can be swapped freely. Battle animations, previously shown only on enemies, can also play on the heroes, and spell effects can be set separately for the caster's side and the target's side.
- **Relevance to Talewright:** not verified. MZ has front-view battles (H12), but the MZ help pages checked do not say whether actor images or animations appear on actors in front view.
- **Source:** https://rpgmakerofficial.com/product/products/rpgxp/battlesys/index.html ; https://rpgmakerofficial.com/product/products/rpgxp/shinkinou/index.html

### RM-OTH-016 — RPG Maker XP: MIDI, WAV and Ogg Vorbis audio
- **What:** Custom audio can be MIDI, WAV or Ogg Vorbis.
- **Relevance to Talewright:** partial. MZ supports only Ogg Vorbis (H21).
- **Source:** https://rpgmakerofficial.com/product/products/rpgxp/tokucho/index.html

### RM-OTH-017 — RPG Maker XP: Panorama (distant-view) backgrounds
- **What:** XP's bundled assets include distant-view (panorama) background graphics for maps, alongside battle backgrounds.
- **Relevance to Talewright:** yes. MZ maps support a parallax background with looping and auto-scroll (H7).
- **Source:** https://rpgmakerofficial.com/product/products/rpgxp/tokucho/index.html

### RPG Maker VX

### RM-OTH-018 — RPG Maker VX: RGSS2 and a script editor inherited from XP
- **What:** VX games are built on RGSS2 (Ruby). Most of the game can be edited with modest programming, and fan scripts are modular paste-ins. VX kept XP's script editor with usability improvements.
- **Relevance to Talewright:** partial. MZ uses JavaScript plugins, not an in-editor Ruby script editor (H23; M3).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx ; https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RM-OTH-019 — RPG Maker VX: A–E tile sets that stack automatically
- **What:** Tiles are split into a base set A plus overlay sets B–E, and the editor stacks them without the user managing layers. This replaced XP's explicit layers.
- **Relevance to Talewright:** yes. MZ uses the same A–E palette and auto-stacks up to two upper tiles (H5).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-vx

### RM-OTH-020 — RPG Maker VX: Passability edited directly on the map tile palette
- **What:** Tile passage settings could be changed right on the tile palette, without opening the database.
- **Relevance to Talewright:** partial. MZ edits passage, 4-direction passage, ladder, bush, counter, damage-floor and terrain-tag settings in the Tileset database screen (H11), not in the map palette.
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-vx

### RM-OTH-021 — RPG Maker VX: Expanded autotiles and automatic shadows
- **What:** Autotiles were extended from coastlines to walls, roofs, ceilings, tall grass and trees, and some tiles cast automatic shadows for depth.
- **Relevance to Talewright:** yes. MZ has A1–A4 autotiles and autoshadows (H5).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html ; https://store.steampowered.com/app/521880/

### RM-OTH-022 — RPG Maker VX: Dungeon auto-generator
- **What:** You pick floor and wall graphics and the editor draws a random room-and-corridor dungeon, which stays editable afterward.
- **Relevance to Talewright:** yes. MZ has "Generate Dungeon" among its aid tools (H4).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html ; https://store.steampowered.com/app/521880/

### RM-OTH-023 — RPG Maker VX: Quick Event creation
- **What:** Common events (transfer, door, treasure chest, inn) are generated from a few choices and remain editable like normal events.
- **Relevance to Talewright:** yes. MZ has Quick Event Creation for Transfer, Door, Treasure and Inn (H15).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-vx

### RM-OTH-024 — RPG Maker VX: 544x416, 32-bit colour, 60 fps
- **What:** VX renders at 544x416 in 32-bit colour at 60 frames per second and bundles a font (UmePlus Gothic) for cleaner message text.
- **Relevance to Talewright:** yes. MZ makes the resolution and fonts configurable (H13).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RM-OTH-025 — RPG Maker VX: Vehicles, standard dash, map looping, parallax
- **What:** Vehicle travel, dashing, looping maps and distant-view parallax backgrounds became built-in features.
- **Relevance to Talewright:** yes. MZ has vehicles (H12), map loop types and parallax (H7), and a per-map dash toggle (H7).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RM-OTH-026 — RPG Maker VX: Faces in messages, icons for items, skills and states
- **What:** Message commands can show face graphics directly, which needed scripting in XP. Status screens show faces, and items, skills, weapons, armour and states get icons.
- **Relevance to Talewright:** yes. MZ's Show Text has a Face field (H17), and the database supports icons (H21 lists IconSet.png).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RM-OTH-027 — RPG Maker VX: Front-view battle on a processed map backdrop, plus battle events
- **What:** Battles are front-view, use a processed image of the current map as the background, and can trigger events mid-battle.
- **Relevance to Talewright:** yes. When no battleback is set, MZ uses a processed map-screen image on non-field maps (H7), and it supports troop battle events (H10).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RM-OTH-028 — RPG Maker VX: Actor options (dual wield, auto battle) and class skill-command names
- **What:** Actors can be set to dual-wield or fight automatically, and each class can rename its skill command in battle.
- **Relevance to Talewright:** yes. In MZ, dual wield and auto battle are Traits (H14).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvx/feature/index.html

### RPG Maker VX Ace

### RM-OTH-029 — RPG Maker VX Ace: RGSS3 (Ruby 1.9.2) with the script editor
- **What:** VX Ace moved to RGSS3, based on Ruby 1.9.2, for faster processing. The MV help confirms that this traditional Script Editor was then removed in MV.
- **Relevance to Talewright:** partial. MZ uses JavaScript plugins only (H23; M3).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx-ace ; https://rpgmakerofficial.com/product/products/rpgvxace/outline/index.html ; https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_03.html

### RM-OTH-030 — RPG Maker VX Ace: Many tilesets per project, chosen per map
- **What:** A project can define up to 999 full tilesets, and each map picks its own. Earlier, every map shared one tileset.
- **Relevance to Talewright:** yes. MZ assigns a tileset per map (H7, H11).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx-ace ; https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html

### RM-OTH-031 — RPG Maker VX Ace: Per-tile attributes, 4-direction passage and terrain tags
- **What:** Bush and counter attributes, previously fixed to set positions, can be assigned to any tile. Tilesets also gained 4-directional passability and terrain tags.
- **Relevance to Talewright:** yes. MZ has Passage (4 dir), Ladder, Bush, Counter, Damage Floor and Terrain Tag 0–7 (H11).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html ; https://www.rpgmakerweb.com/products/rpg-maker-vx-ace

### RM-OTH-032 — RPG Maker VX Ace: Region IDs (up to 63 per map)
- **What:** A region-ID layer marks arbitrary map areas, for example to restrict which encounters can occur there. VX Ace allowed up to 63 regions per map.
- **Relevance to Talewright:** yes, expanded. MZ supports region IDs 1–255 and per-troop region scopes (H7). The MV help records the jump from 63 to 255 (M2).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx-ace ; https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_02.html

### RM-OTH-033 — RPG Maker VX Ace: Shadow tool for autoshadows
- **What:** Users can draw shadows by hand or erase the ones generated automatically.
- **Relevance to Talewright:** yes. MZ has the Shadow Pen, in quarter-tile steps (H5).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx-ace

### RM-OTH-034 — RPG Maker VX Ace: Features (traits) and use-effects system
- **What:** A general "features" list can be attached to actors, classes, weapons, armour and states. Items and skills get "use effects". Element and state resistances become numeric rates.
- **Relevance to Talewright:** yes. MZ has Traits and Effects (H14).
- **Source:** https://www.rpgmakerweb.com/products/rpg-maker-vx-ace ; https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html

### RM-OTH-035 — RPG Maker VX Ace: Damage formulas in the database
- **What:** Skill and item damage can be written as a formula directly in the database entry.
- **Relevance to Talewright:** yes. MZ skills and items have a Formula field (H9).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html

### RM-OTH-036 — RPG Maker VX Ace: Built-in character generator
- **What:** A bundled generator builds walking sprites and faces from parts (hair, outline, clothing), expanding an earlier web-based tool.
- **Relevance to Talewright:** yes. MZ includes a Character Generator (H4).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html ; https://store.steampowered.com/app/220700/

### RM-OTH-037 — RPG Maker VX Ace: Followers on the map and parties larger than four
- **What:** Party members can walk in formation behind the leader, and parties can exceed four. Only the first four appear in formation and in battle; the rest are reserves.
- **Relevance to Talewright:** yes. MZ has "Show Player Followers" and EXP for reserve members (H12).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html

### RM-OTH-038 — RPG Maker VX Ace: TP resource and two-part battlebacks
- **What:** VX Ace added a TP parameter for special skills, and battle backgrounds composed from two images.
- **Relevance to Talewright:** yes. MZ shows TP (H12) and builds battlebacks from two images (H7).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html

### RM-OTH-039 — RPG Maker VX Ace: Ogg Theora video playback via events
- **What:** An event command can play movies in Ogg Theora format.
- **Relevance to Talewright:** partial. MZ has Play Movie (H19), but its formats are WebM and MP4/H.264 (H21).
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/feature/index.html ; https://store.steampowered.com/app/220700/

### RM-OTH-040 — RPG Maker VX Ace: 100+ sample maps loadable as new maps
- **What:** More than 100 sample maps (fields, towns, dungeons, interiors) can be loaded as the starting point for a new map.
- **Relevance to Talewright:** yes. MZ has "Load Sample Map" (H6). MZ's count is not verified.
- **Source:** https://rpgmakerofficial.com/product/products/rpgvxace/outline/index.html

### RM-OTH-041 — RPG Maker VX Ace: Single portable EXE for Windows
- **What:** A finished game exports to a portable EXE that runs on Windows.
- **Relevance to Talewright:** partial. MZ deploys a folder (H22).
- **Source:** https://store.steampowered.com/app/220700/ ; https://www.rpgmakerweb.com/products/rpg-maker-vx-ace

### RPG Maker MV

### RM-OTH-042 — RPG Maker MV: HTML5 output to Windows, macOS, Linux, Android, iOS and web
- **What:** MV games are HTML5. The deploy dialog targets Windows, Mac OS X and Linux, and browser play works by uploading to a web server. The help documents wrapping the web build as Android APK (Crosswalk) and iOS IPA (Cordova) apps.
- **Relevance to Talewright:** partial. The MZ help lists output for Windows, macOS and web browsers only, though web builds run in mobile browsers (iOS 12+, Android 8+) (H22).
- **Source:** https://store.steampowered.com/app/363890/ ; https://rpgmakerofficial.com/product/MV_Help/page/01_11_04.html ; https://rpgmakerofficial.com/product/MV_Help/page/01_11_05.html ; https://rpgmakerofficial.com/product/MV_Help/page/01_11_06.html

### RM-OTH-043 — RPG Maker MV: Editor on Windows, Mac and Linux/SteamOS
- **What:** MV introduced a Mac version of the editor alongside Windows, and the help documents installing the editor on Linux/SteamOS.
- **Relevance to Talewright:** partial. The MZ editor supports Windows 8.1/10 (64-bit) and macOS 10.13+ only (H3).
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_01.html ; https://rpgmakerofficial.com/product/MV_Help/page/01_11_14.html ; https://store.steampowered.com/app/363890/

### RM-OTH-044 — RPG Maker MV: Side-view battle switch with actor motions and state overlays
- **What:** One System setting flips battles between front view and side view. Side view adds actor battler sheets, attack and chant motions, and state motions and overlays.
- **Relevance to Talewright:** yes. MZ offers front-view and side-view (H12).
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_05.html ; https://store.steampowered.com/app/363890/

### RM-OTH-045 — RPG Maker MV: JavaScript plugins and Plugin Manager replace the script editor
- **What:** MV dropped the Ruby script editor. Behaviour is extended by dropping .js plugin files into the project and enabling, ordering and configuring them in a Plugin Manager. Official plugins are bundled.
- **Relevance to Talewright:** yes. MZ keeps this model, adding pull-down plugin commands and load-order errors (H23; S10).
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_03.html ; https://store.steampowered.com/app/363890/

### RM-OTH-046 — RPG Maker MV: 816x624 default resolution (1.5x VX Ace)
- **What:** The game resolution rose from 544x416 to 816x624.
- **Relevance to Talewright:** yes. MZ's resolution is configurable (H13).
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_02.html ; https://store.steampowered.com/app/363890/

### RM-OTH-047 — RPG Maker MV: Automatic second upper tile, region tab, map-to-image export
- **What:** Two upper-layer tiles can overlap automatically. The region editor moved into a tile-palette tab with 255 IDs. Maps can be saved as images from the map list.
- **Relevance to Talewright:** yes. MZ has the same two-upper-tile rule (H5) and 255 regions (H7), and its official page cites improved map image saving (S10).
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_02.html

### RM-OTH-048 — RPG Maker MV: Larger database caps (2,000 entries)
- **What:** Skills, items, weapons, armour, enemies and troops can each have up to 2,000 entries, which the official page says is twice the VX Ace limit.
- **Relevance to Talewright:** not verified. The MZ help pages checked state no maximum.
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_04.html ; https://www.rpgmakerweb.com/products/rpg-maker-mv

### RM-OTH-049 — RPG Maker MV: Mouse and touch play
- **What:** Games can be played with a mouse or a touch screen as well as the keyboard.
- **Relevance to Talewright:** yes. MZ adds browser and touch control schemes and a smartphone-friendly UI by default (H1).
- **Source:** https://store.steampowered.com/app/363890/ ; https://www.rpgmakerweb.com/products/rpg-maker-mv

### RM-OTH-050 — RPG Maker MV: Runtime conveniences (window resize, fast-forward, F8 dev tools, CSS fonts, 20 save slots)
- **What:** Players can resize the game window and speed up events by holding the action key. Developers get F8 developer tools during playtest and can change fonts via a CSS file. Save slots rose from 16 to 20.
- **Relevance to Talewright:** partial. MZ documents developer tools (H4) and sets fonts in the editor (H13). The other items were not checked in MZ help.
- **Source:** https://rpgmakerofficial.com/product/MV_Help/page/01_01_01_08.html

### RPG Maker Unite

### RM-OTH-051 — RPG Maker Unite: Runs inside the Unity Editor with source access
- **What:** Unite is a no-code RPG editor built on and running in the Unity Editor, and it requires a Unity ID. Users can switch to the Unity Editor UI at any time and read or modify most of the runtime source (some DLLs excepted). The official pages say this is not officially supported.
- **Relevance to Talewright:** no. MZ is a standalone editor; its runtime JavaScript is customised through plugins (H23).
- **Source:** https://store.steampowered.com/app/1650950/ ; https://store.rpgmakerofficial.com/products/rpg-maker-unite ; https://rpgmakerunite.com/en/

### RM-OTH-052 — RPG Maker Unite: Unity build pipeline (Windows/Android, later macOS/iOS/WebGL)
- **What:** Builds go through Unity. The official store lists Windows and Android as supported, with macOS, iOS and WebGL planned. The pre-release Steam page listed Windows, macOS, Android and iOS. A 2025 manual article documents WebGL builds as a beta. Other targets depend on the user's own Unity and platform-holder agreements.
- **Relevance to Talewright:** partial. MZ deploys to Windows, macOS and web (H22).
- **Source:** https://store.rpgmakerofficial.com/products/rpg-maker-unite ; https://store.steampowered.com/app/1650950/ ; https://rpgmakerunite.com/en/article/42285977040281/

### RM-OTH-053 — RPG Maker Unite: Native Full HD (1920x1080)
- **What:** Unite is described officially as the first entry in the series with native 1920x1080 resolution.
- **Relevance to Talewright:** partial. MZ lets the screen size be configured (H13), but the help does not describe FHD as a native target.
- **Source:** https://store.steampowered.com/app/1650950/ ; https://rpgmakerunite.com/en/article/6932492229657/

### RM-OTH-054 — RPG Maker Unite: Free-form character animation frames
- **What:** The old cap of 3 walking patterns is removed. Creators set any number of frames and patterns, as long as the sheet fits the maximum image size.
- **Relevance to Talewright:** no. MZ character sheets are fixed at 4 directions x 3 patterns (H21).
- **Source:** https://rpgmakerunite.com/en/article/6932492229657/ ; https://store.steampowered.com/app/1650950/

### RM-OTH-055 — RPG Maker Unite: Layered map model with Large Parts, a background image and collision
- **What:** Maps have tile layers A–D, a single large Background image, a Parallax layer and a Collision layer. Collision uses numbered tiles 1–16 with passage settings, and the A–D layers override it. "Large Parts" are tiles up to 2048x2048, against standard tiles of 98x98 image pixels, for placing buildings and trees in one go. The map grid is 96 px. Whole illustrations can be used as maps with collision on top.
- **Relevance to Talewright:** partial. MZ has 4 tile layers, parallax and a 48px grid (H5, H7, H21), but no collision layer, background-image map or oversized tile parts.
- **Source:** https://rpgmakerunite.com/en/article/14950935540377/ ; https://rpgmakerunite.com/en/about/

### RM-OTH-056 — RPG Maker Unite: Per-tile registration and Tile Groups
- **What:** Individual tiles are registered as project assets, each with its own passability, directional blocking, ladder, bush and damage-floor flags. Custom tile groups can be saved and loaded per map.
- **Relevance to Talewright:** partial. MZ sets these flags per tile within a tileset (H11) and has no reusable tile-group concept.
- **Source:** https://rpgmakerunite.com/en/article/6932492229657/ ; https://rpgmakerunite.com/en/article/14950935540377/

### RM-OTH-057 — RPG Maker Unite: Outline Editor (story structure view)
- **What:** A planning view organises the game into chapters, then sections (groups of maps), then maps. It draws story lines between them and auto-generated "switch lines" showing which sections are linked by a switch. It was discontinued in v1.2.0.
- **Relevance to Talewright:** no. MZ has only a map tree (H6).
- **Source:** https://rpgmakerunite.com/en/article/13937815423513/ ; https://store.steampowered.com/app/1650950/

### RM-OTH-058 — RPG Maker Unite: Auto-Guide stat balancing
- **What:** From a few inputs (estimated encounter level, turns to defeat an actor, turns to be defeated, magic strength), the Auto-Guide suggests enemy HP, stats, EXP and gold. A class-based simulator does the same for actor stats.
- **Relevance to Talewright:** no. MZ offers class EXP and parameter curves but no balancing assistant (H8).
- **Source:** https://rpgmakerunite.com/en/article/9517190933401/ ; https://rpgmakerunite.com/en/article/6932492229657/ ; https://rpgmakerunite.com/en/about/

### RM-OTH-059 — RPG Maker Unite: UI Settings editor
- **What:** A database screen picks one of three UI layout patterns (with optional horizontal flip) and sets fonts, colours and nine-slice window, frame and button images. It can also strip menu commands (for example Save) and hide HP, MP or TP in battle, with live previews.
- **Relevance to Talewright:** partial. MZ has window colour, menu-command toggles and font settings (H12, H13), but no layout patterns or nine-slice skinning UI.
- **Source:** https://rpgmakerunite.com/en/article/9505520588185/

### RM-OTH-060 — RPG Maker Unite: Add-ons as Unity packages
- **What:** Add-ons, Unite's equivalent of plugins, are imported as .unitypackage through Unity. An Add-on Manager toggles them, orders them by drag-and-drop and edits parameters. Add-on commands can be called from events, but only work in Windows builds.
- **Relevance to Talewright:** partial. MZ has the same manager concepts for JavaScript plugins (H23, H20).
- **Source:** https://rpgmakerunite.com/en/article/13572542408729/ ; https://store.steampowered.com/app/1650950/

### RM-OTH-061 — RPG Maker Unite: "Call Unity Scene" event command
- **What:** An event can hand control to a custom Unity scene, optionally unloading the map scene. A single integer variable passes in, and a result is written back when the scene returns.
- **Relevance to Talewright:** no. There is no MZ equivalent; the closest is the JavaScript Script command (H20).
- **Source:** https://rpgmakerunite.com/en/article/35475324025497/

### RM-OTH-062 — RPG Maker Unite: "Post Effects" command and encounter wipes
- **What:** A command stacks up to 10 full-screen effects: glitch, blur, bloom, colour or gradient fill with blend modes, and mosaic. Effects fade in and out and can be previewed in the editor. Battle-transition wipes (circle, horizontal, vertical) are chosen in the database.
- **Relevance to Talewright:** no. MZ screen commands are tint, flash, shake and weather (H18).
- **Source:** https://rpgmakerunite.com/en/article/35461927757977/

### RM-OTH-063 — RPG Maker Unite: Battle setup extras (enemy size classes, side-view auto-alignment, random troop spawning)
- **What:** Enemies are tagged Small, Large or Boss, with side-view caps of 6, 4 or 1. Side-view enemy groups auto-align, which the official page says MV only did in front view. Several troops can be registered on a map and appear at random.
- **Relevance to Talewright:** partial. MZ has weighted random troop lists per map or region (H7) and manual troop placement (H10). It has no size classes, and side-view auto-alignment is not verified.
- **Source:** https://rpgmakerunite.com/en/article/9517190933401/ ; https://rpgmakerunite.com/en/article/6932492229657/

### RM-OTH-064 — RPG Maker Unite: Not MZ's successor, with no MZ project or plugin compatibility
- **What:** The official pages say Unite does not succeed MZ, cannot open or convert past projects, and cannot use past plugins. MZ remains the recommended tool for existing projects.
- **Relevance to Talewright:** n/a (a compatibility statement). It is a useful precedent for Talewright's own import story.
- **Source:** https://rpgmakerunite.com/en/about/ ; https://rpgmakerunite.com/en/article/6932492229657/

### RPG Maker WITH (console)

### RM-OTH-065 — RPG Maker WITH: Console-native editor (Switch, PS4, PS5)
- **What:** WITH is a full RPG editor for home consoles. Its map editor is retuned for console use, with more screen space and a palette row of recently used tiles, and shortcuts jump between the map, database and event editors.
- **Relevance to Talewright:** no. MZ is a PC/Mac editor (H3).
- **Source:** https://with.rpgmakerofficial.com/ ; https://with.rpgmakerofficial.com/product/

### RM-OTH-066 — RPG Maker WITH: Online asset sharing across platforms
- **What:** Creators upload and download maps, database records (items, weapons, armour, skills, enemies) and common events over the internet, and drop them into their own projects. Shared assets and games work across Switch, PS5 and PS4.
- **Relevance to Talewright:** no. MZ has no built-in sharing service.
- **Source:** https://with.rpgmakerofficial.com/ ; https://with.rpgmakerofficial.com/product/

### RM-OTH-067 — RPG Maker WITH: Publish and play games in-platform, plus a free player app
- **What:** Finished games are posted online and played in WITH itself or in the free "RPG MAKER WITH PLAYER", which also has a trial of the creation tools. Games made in the trial carry over to the full version.
- **Relevance to Talewright:** no. MZ deploys files and has no hosted publishing (H22).
- **Source:** https://with.rpgmakerofficial.com/

### RM-OTH-068 — RPG Maker WITH: No custom asset import, and no MV Trinity carry-over
- **What:** Console rules mean users cannot import their own illustrations or free assets; only built-in, DLC and shared assets are usable. Data from the earlier console MV Trinity cannot be brought over.
- **Relevance to Talewright:** n/a. This is the opposite of MZ, which imports user assets (H21).
- **Source:** https://with.rpgmakerofficial.com/

### RM-OTH-069 — RPG Maker WITH: Custom systems via editable common events
- **What:** The official page suggests modifying shared common events to build bespoke game systems without code.
- **Relevance to Talewright:** yes. MZ has common events (H16).
- **Source:** https://with.rpgmakerofficial.com/

### RPG MAKER U2U (announced 2026; unreleased)

### RM-OTH-070 — RPG Maker U2U: P2D (Perspective 2D) maps with real elevation
- **What:** P2D keeps the classic fixed top-down RPG Maker view but gives maps true height. Slopes, ledges, multi-storey buildings and overpasses are possible.
- **Relevance to Talewright:** no. MZ maps are flat 2D tile grids; height is only faked with passage settings (H11).
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/ ; https://u2u.rpgmakerofficial.com/en/feature/ ; https://www.gotchagotcha.jp/assets/pdf/20260521_RPGMAKERU2U.pdf

### RM-OTH-071 — RPG Maker U2U: P2D Map Editor (prefab placement and block building)
- **What:** In quick mode you place ready-made objects from a list. In detailed mode you build new objects from blocks to form complex shapes and uneven terrain, then decorate them with ordinary 2D map tiles. Gotcha Gotcha Games says it needs no 3D modelling knowledge.
- **Relevance to Talewright:** no. MZ's editor paints 2D tiles only (H5).
- **Source:** https://u2u.rpgmakerofficial.com/en/feature/ ; https://www.gotchagotcha.jp/assets/pdf/20260521_RPGMAKERU2U.pdf ; https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/

### RM-OTH-072 — RPG Maker U2U: Reuse of MV/MZ tile assets
- **What:** U2U keeps the series' map-tile standards, so 2D tiles made for MV and MZ can decorate P2D objects.
- **Relevance to Talewright:** n/a (an asset-compatibility statement). It shows that MZ tile standards remain the series' interchange format.
- **Source:** https://u2u.rpgmakerofficial.com/en/feature/ ; https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/

### RM-OTH-073 — RPG Maker U2U: Lighting, shadows, depth of field and fog
- **What:** Built-in effects include camera depth of field, rain and fog weather, and dynamic lights and shadows such as interior lights and street lamps. They are combined with P2D height for mood.
- **Relevance to Talewright:** partial. MZ has rain, storm and snow weather and screen tint (H18), but no lights, shadows, fog or depth of field.
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/

### RM-OTH-074 — RPG Maker U2U: Automatic silhouettes for hidden characters
- **What:** Characters behind walls or structures are automatically drawn as silhouettes.
- **Relevance to Talewright:** no. MZ's "star" passage tiles simply draw over characters (H11).
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/

### RM-OTH-075 — RPG Maker U2U: 100+ prebuilt P2D maps and a bundled asset set
- **What:** U2U ships with over 100 ready-made P2D maps, plus characters, monsters, visual effects, music and sound effects, so no extra downloads are needed.
- **Relevance to Talewright:** partial. MZ ships sample maps and assets (H6, S10), but not P2D content.
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/ ; https://u2u.rpgmakerofficial.com/en/feature/ ; https://www.gotchagotcha.jp/assets/pdf/20260521_RPGMAKERU2U.pdf

### RM-OTH-076 — RPG Maker U2U: MZ-based workflow for events and battles; no project or plugin conversion
- **What:** Maps, events and battles are configured without code in a UI and UX modelled on MZ. Screenshots of an Event Editor and a Database were shown. Converting projects or plugins from earlier RPG Makers is explicitly not supported.
- **Relevance to Talewright:** n/a. U2U uses MZ as its design baseline, the same as Talewright.
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/ ; https://u2u.rpgmakerofficial.com/en/feature/ ; https://rpgmakerofficial.com/en/news/4265/

### RM-OTH-077 — RPG Maker U2U: Announced platform and requirements
- **What:** Steam lists Windows 11 64-bit, a DirectX 11 GPU, 8 GB RAM and 10 GB of storage, with 12 interface languages. The release date is shown as "Coming soon". A playable P2D Map Editor demo was shown at BitSummit PUNCH (22–24 May 2026).
- **Relevance to Talewright:** partial. The MZ editor runs on Windows 8.1/10 and macOS (H3).
- **Source:** https://store.steampowered.com/app/4856440/RPG_MAKER_U2U/ ; https://www.gotchagotcha.jp/assets/pdf/20260521_RPGMAKERU2U.pdf

---

## 4. Not verified

No official page loaded during this research confirms the following. Each needs an official source before it is used.

1. **RPG Maker 2003: active-time battle (ATB) gauges, Active vs Wait modes, and battle-layout types** ("traditional / alternative / gauge"). No official product page, store page or help mentions ATB. Leads exist only in a Steam Community changelog thread posted by a user without a developer badge (https://steamcommunity.com/app/362870/discussions/0/618460171314070796/) and in forum or third-party posts. These are not official.
2. **RPG Maker 2003: dedicated battle-event pages and the 1.10/1.12 English-release changes** (pictures in battle, picture limit 50 to 1000, faster ATB). These appear only in the same non-official thread and in EasyRPG notes.
3. **RPG Maker 2003: original Japanese release year.** No official 2003 JP page exists (HTTP 404), and the English pages give only the 2015 Steam release.
4. **RPG Maker 2000: exact original release date.** Only "debuted ~15 years before 2015" (Steam) and copyright years on the JP page were found.
5. **RPG Maker XP: per-tile priority (0–5) in the tileset database**, which lets characters walk behind tall tiles. It was found only in forum and fan-wiki pages; the official XP pages mention only three layers.
6. **RPG Maker XP: fog graphics and per-map fog settings.** Not mentioned on any official XP page loaded. Only panorama (distant-view) graphics are confirmed.
7. **RPG Maker XP: whether the script editor is a separate in-editor window in XP itself.** This is inferred only from the VX JP page, which says VX "carried over" XP's script editor; the XP pages describe RGSS editing without naming the tool.
8. **RPG Maker 2000/2003: two-layer (lower/upper chipset) maps.** This is implied only by the XP pages saying layers went "from two to three"; no 2000/2003 page states it.
9. **RPG Maker Unite: Add-on implementation language (C#).** Official pages say only Unity and .unitypackage, without naming the language.
10. **RPG Maker Unite: exact Asset Store launch day.** Official sources conflict: 27 Apr 2023 (S49) vs 8 May 2023 (S51).
11. **RPG Maker WITH: Western (NA/EU) release dates and English feature pages.** These appear only in news sites; no official English WITH page was found (with.rpgmakerofficial.com/en/ returns 404, and rpgmakerweb has no WITH page).
12. **RPG Maker WITH: battle system type and view, editor limits, and input methods.** Not described on the official JP pages.
13. **RPG Maker U2U: release date, underlying engine, export platforms, plugin or scripting support, and battle-system details.** None are stated on the official pages. The Producer Letters with further detail are hosted on x.com and were not fetched.
14. **RPG Maker MZ comparison gaps:** MZ's maximum database entry counts (vs MV's 2,000), whether MZ shows actors and animations on actors in front-view battles (vs XP), MZ side-view enemy auto-alignment (vs Unite), and whether MZ supports a designated transparent colour key (vs XP). The MZ help pages checked do not state these.
