# Review — Epic 7 (assets: generated defaults, autotiles, tile flags, tileset form, resolver, import tools)

Reviewers: two fresh-context subagents, 2026-10-14, read-only, on `git diff 9e26221d20..HEAD` (commits 7.1.1–7.5)
against spec 05, D-005, D-006, D-011, D-012, D-031, D-034 and the RM-TIL/RM-AST inventory rows
(`plan/research/rpgmaker/02_maps_tilesets_assets.md`). Part A: asset generator, autotile core, importer, resolver
(tasks 7.1, 7.2, 7.4): 9 findings, none high. Part B: tile flags, tileset form, bridge tools (tasks 7.3, 7.5): 14
findings, none high.

## Part A (fixed in 7.6.1)

Confirmed fine by the reviewer: the mask math is consistent across the generator, `JRPGAutotile`, the importer's
peering bits and the resolver; the animation layout and the importer's creation order fit Godot's atlas rules
(`scene/resources/2d/tile_set.cpp:5055-5150,5203,5305`); every failed import changes nothing; the resolver's wrap,
seam and malformed-input handling, determinism and core boundary; the brute-force reference shares no code with the
resolver; generator determinism; asset sizes and the icon table; the `.ogg.import` sidecars; CC0 licensing and no RM
material.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | med | `generate_assets.py --check` compared the in-memory output with the manifest only: an edited PNG, a deleted `.import` or a changed import setting passed; regenerating trusted the manifest and never restored them; not run by any check | fixed: `--check` also compares the committed files (each PNG's pixels, each Ogg's length and channels, each `.import` text, stray files); sound entries record every import setting; regenerating rewrites what differs on disk (Ogg only when its samples change, so no serial-number churn) and recreates sidecars; `static_checks.py` runs it ("generated assets", skipped when a library is missing); verified by editing a PNG and deleting a sidecar (both reported, regenerating restored them byte for byte) |
| 2 | low-med | The resolver's flag bits do not line up with `JRPGTilesetData::EdgeMode` or spec 05's scroll-type order, with no helper: future map code could swap axes | fixed: `JRPGAutotileResolver.get_flags(loop_horizontal, loop_vertical, edge_mode)` (bound, documented); spec 05 states the scroll-type mapping (type & 2 horizontal, type & 1 vertical); doctest over the four scroll types |
| 3 | low | `import_sheet` accepted any integer kind and frames < 1 from scripts | fixed: unknown kinds and frames < 1 are refused with a reason; doctests |
| 4 | low | `get_blob_cell` / `get_wall_cell` had no range checks (47 gave the preview cell, 16 wrapped) | fixed: `ERR_FAIL_INDEX_V`, (-1, -1) out of range, documented; doctests |
| 5 | low | Animation speed hard-coded and undocumented | answered: kept at 3 frames per second for every animated sheet (Godot's atlas animation speed 3, frame duration 1 s), now documented in spec 05 and the template guide; a per-entry speed can come with the map editor's animation settings if needed |
| 6 | low | Bit masks bound as plain enums; static-only classes instantiable | fixed: `Neighbor`, `Side` and the resolver's `Flags` are bitfields (`BIND_BITFIELD_FLAG`, `BitField<>` parameters, `is_bitfield` in the docs); `JRPGAutotile`, `JRPGAutotileResolver`, `JRPGAutotileImporter` and `JRPGTileFlags` are registered abstract |
| 7 | low | Test gaps: peering bits of 2 shapes only, no animated wall sheet, script wrappers untested, `changed` checked one way, "two layers" trivially true | fixed: every blob (47) and wall (16) shape's peering bits against its mask; an animated wall sheet (separation (3, 0), frame region); `import_sheet_bind` and `update_around_bind`; `changed` lists exactly the cells whose shape changed (plus the edited cell). The two-layer case stays as the documentation of a per-layer API (layers cannot influence each other by construction) |
| 8 | low | D-034's reversal ("change `TILE` and regenerate") would draw broken art: the painters use fixed 48 px geometry | fixed: the reversal names what must scale by `TILE / 48` first |
| 9 | low | Doc nits: ambiguous "separation = one block", a citation off by a few lines, "generated" table comment, British spellings, no pinned library versions | fixed: separation explained (one block minus one tile, 7 and 3), `tile_set.h:218-235`, "kept equal" comments, US spellings in docs and comments, `talewright/tools/asset_gen/requirements.txt` pins numpy, Pillow and soundfile |
