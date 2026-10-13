# RPG Maker lineup, baseline choice, Godot trademark policy (planning notes, 2026-10-13)

## RPG Maker lineup (official sources)
- Official news index https://rpgmakerofficial.com/en/news/ (fetched 2026-10-13) lists, 2025–2026:
  - 2026-08-27 "RPG MAKER U2U Steam Store Page Now Live! Wishlisting Now Open!" (https://rpgmakerofficial.com/en/news/4265/)
  - 2026-07-24 "The official pixel art tool, Tileset Builder, is here!" (https://rpgmakerofficial.com/en/news/4244/)
  - 2026-06-15 "Announcing the newest entry in the RPG Maker family, 'RPG Maker U2U'! New Map Creation Features
    Revealed" (https://rpgmakerofficial.com/en/news/4181/) — the page itself does not list the features;
    platform/engine/release date **not verified**; status: announced, wishlist only ⇒ **not released**.
  - 2026-03-26 teaser for a new project; 2025 items about RPG MAKER UNITE (game jam, macOS support, sales).
- Official store https://store.rpgmakerofficial.com lists RPG Maker MZ, RPG Maker Unite, ACTION GAME MAKER
  (described as built on the Godot Engine) and Pixel Game Maker MV (search result summary, 2026-10-13).
- RPG Maker MZ official online help: https://rpgmakerofficial.com/product/MZ_help-en/ (RoboHelp; TOC crawled into
  `plan/research/mz_help_toc.md` by `plan/tools/crawl_mz_toc.py`); API documentation link in the TOC:
  https://developer.rpgmakerweb.com/rpg-maker-mz/.

## Baseline decision
- **Baseline = RPG Maker MZ**: the most complete *released* standalone editor with complete official online
  documentation (55+ help pages covering map, database, events, all event commands, documentation for asset
  standards, plugins). Unite is Unity-based (not standalone in the same sense); U2U is unreleased and its
  features are not verified. Notable features of MV/VX Ace/XP/2003/Unite are inventoried separately (task 2.1.8).

## Godot trademark policy
- Godot Foundation, "GODOT TRADEMARK POLICY AND LICENSE", version dated 2025-07-21,
  https://godot.foundation/policies-and-procedures/trademark-policy (fetched 2026-10-13). Key points:
  - Commercial game engines / commercial forks using the marks need a custom license.
  - The GODOT word mark may appear in a project/product name only together with a word describing its purpose,
    never in isolation; not in company/trading names.
  - The Foundation reserves the exclusive right to use the GODOT logo as a logo; derivatives must devise their
    own logo (logo use in a game's splash screen is allowed).
  - It must be clear the use is not affiliated with or endorsed by the Foundation; recommended disclaimer:
    "not affiliated with or endorsed by the Godot Foundation" and "uses the GODOT® name and logos under a
    permissive license" (not a hard requirement).
- Consequence: product name **Talewright**, described as "a JRPG-focused fork of the Godot Engine"; disclaimer
  in `TALEWRIGHT.md`; no RPG Maker trademarks in any name (D-003).

## Asset licensing data point
- Kenney assets are CC0: "all game assets on the asset pages are public domain licensed (CC0)", attribution not
  required (https://kenney.nl/support, fetched 2026-10-13). Not used by default (D-011: generated originals).
