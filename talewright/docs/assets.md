# Default assets

Talewright's default tiles, autotiles and objects are drawn by code (D-011), so they are original, free to use
(CC0, `talewright/assets/LICENSES.md`) and can be regenerated exactly. They live in `talewright/assets/generated`.

## Regenerating and checking
```
python talewright/tools/generate_assets.py           # write the images and manifest.json
python talewright/tools/generate_assets.py --check   # regenerate in memory and compare with manifest.json
```
Needs Python with numpy and Pillow. `manifest.json` records each image's size and the SHA-256 of its RGBA pixels
(not of the PNG file, which depends on the compressor) and the numpy and Pillow versions used: the same versions give
the same pixels.

## What is generated
Tiles are 48 × 48 pixels (D-034).

| File | Contents |
|---|---|
| `autotiles/grass.png`, `dirt_path.png`, `sand.png` | Terrain autotiles: 47-tile blob sheets, 8 × 6 tiles; tile *i* is shape *i* (spec 05), the last cell a preview |
| `autotiles/water.png` | Animated water: three 8 × 6 blocks side by side, one per frame |
| `walls/stone_wall.png`, `brick_wall.png`, `wood_wall.png`, `roof.png` | Wall faces and a roof: 4 × 4 sheets indexed by the 4-neighbour mask (N 1, E 2, S 4, W 8) |
| `tiles/outdoor.png` | 8 × 4 tiles: ground fills (grass, dark grass, dirt, sand, cobblestone, wood floor, stone tiles, water); flowers, rocks, grass tuft, mushrooms, leaves, puddle, cracks; a round tree and a pine (crown above, trunk below), a lamp post (two tiles), signpost, bush, stump, barrel, crate, chest, pot, fences, stone steps |
| `tiles/buildings.png` | Door, windows (dark and lit), flower box, table, chair, rug, parquet, a bookshelf and a bed (two tiles each), stone tiles |
| `characters/<name>.png` | Six characters (hero, mage, knight, villager_man, villager_woman, merchant): 192 × 192 sheets of 48 × 48 frames, rows facing up, right, down, left and 4 walk frames each, column 0 standing (spec 05) |
| `faces/<name>.png` | A 144 × 144 portrait of each character |
| `enemies/<name>.png` | Front-view battlers on transparent backgrounds: moss_blob (the starter Moss Blob), cave_bat, mushroom_imp, thorn_wolf, stone_golem, ember_wisp |
| `battlebacks/<scene>_back.png`, `<scene>_ground.png` | 1280 × 720 battlebacks for grassland, forest and cave: an opaque backdrop and a ground layer in perspective, transparent above the horizon, so either half can be reused |
| `system/icons.png` | The icon set: 32 × 32 icons, 16 per row; an entry's `icon_index` is the position (table below) |
| `system/window.png`, `window_cursor.png`, `window_arrows.png` | The window skin (below) |

Terrain textures repeat every tile, and autotile edges are shaped by waves that repeat every tile too, so
neighboring tiles join without seams. Autotiles draw only their terrain: the area outside it is transparent, so a
terrain is painted over a ground layer.

## Icon indexes
Icon *i* is at column *i* mod 16, row *i* div 16 of `system/icons.png`. Index 0 is empty (no icon).

| Indexes | Icons |
|---|---|
| 0–15 | none, potion, ether, herb, elixir, antidote, scroll, key, gem, money_bag, bread, tent, feather, bomb, map, book |
| 16–31 | sword, dagger, axe, spear, bow, staff, mace, claw, shield, helmet, armor, robe, boots, gloves, ring, amulet |
| 32–47 | physical, fire, ice, thunder, water, earth, wind, light, dark, poison, sleep, stun, silence, confusion, knocked_out, guard |
| 48–63 | attack_up, defense_up, magic_up, agility_up, attack_down, defense_down, magic_down, agility_down, regen, star, coin, heart, sparkle, flag, question, none_63 |

## Window skin
- `system/window.png`: a 96 × 96 nine-slice window, 16-pixel margins on every side (corners stay, edges and the
  translucent body stretch); for a Godot `StyleBoxTexture` set all four texture margins to 16.
- `system/window_cursor.png`: the selection highlight, 48 × 48 nine-slice with 8-pixel margins.
- `system/window_arrows.png`: four 16 × 16 marks side by side: scroll up, scroll down, more text (the pause mark of
  a message), and a right arrow for submenus.
