# Godot 4.7.2: runtime systems reusable for JRPG (tiles, resources, inspector/undo, Expression, i18n, input, UI, audio, persistence, effects)

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

Everything below was checked against the source at tag **4.7.2-stable** (`gh api` tree plus raw file fetches). All path:line references point at that tag. All requested paths exist as named. The tile editor lives in `editor/scene/2d/tiles/`.

**Things that matter most for the design:**
- **Gamepad confirm/cancel:** `ui_accept` and `ui_cancel` have no gamepad buttons by default (§6).
- **Formula safety:** `Expression`'s `const_calls_only` does not stop calls to script methods (§4).
- **Tile editor:** it cannot be extended (§1).
- **Autotiles:** Godot terrains do not build tiles from RPG Maker-style quarter-tiles (§1).

---

## 1. TILEMAPS

**TileMapLayer** (`scene/2d/tile_map_layer.h`, class at :332, extends Node2D)
- `set_cell(coords, source_id=-1, atlas_coords=(-1,-1), alternative_tile=0)` :557, bound at .cpp:2187. `erase_cell` :558.
- `get_cell_source_id` / `get_cell_atlas_coords` / `get_cell_alternative_tile` / `get_cell_tile_data` :562-565, bound .cpp:2192-2195.
- `get_used_cells`, `get_used_cells_by_id` :567-568. `is_cell_flipped_h/v`, `is_cell_transposed` :571-573. `get_pattern` / `set_pattern` :576-577.
- Terrain (autotile) calls: `set_cells_terrain_connect(cells, terrain_set, terrain, ignore_empty_terrains=true)` :580 and `set_cells_terrain_path(...)` :581, bound .cpp:2210-2211, implemented .cpp:3038 and :3077. The internal `terrain_fill_connect/path/pattern` are marked "Not exposed" (:543-545).
- Coordinate helpers: `map_to_local` / `local_to_map` :600-601, `get_surrounding_cells` :598, `get_neighbor_cell` :599, `get_coords_for_body_rid` :586.
- Runtime hooks you can override in script: `_use_tile_data_runtime_update(coords)`, `_tile_data_runtime_update(coords, tile_data)`, `_update_cells(coords, forced_cleanup)` :592-594, bound .cpp:2268-2270. `notify_runtime_tile_data_update()` :591.
- Properties: `enabled`, `occlusion_enabled`, `y_sort_origin`, `rendering_quadrant_size`, `collision_enabled`, `use_kinematic_bodies`, `physics_quadrant_size`, `navigation_enabled` (.cpp:2274-2288). Cells are stored as `tile_map_data` PackedByteArray (.cpp:2272). There is a `changed` signal (.cpp:2292).

**TileSet** (`scene/resources/2d/tile_set.h`, class at :149)
- `CellNeighbor` (16 peering bits) :218-236.
- `TerrainMode`: `TERRAIN_MODE_MATCH_CORNERS_AND_SIDES`, `_MATCH_CORNERS`, `_MATCH_SIDES` :240-244. Terrain set API: `add_terrain_set` :467, `set_terrain_set_mode` :470, `add_terrain` :475, `set_terrain_name` / `set_terrain_color` :479-481, `is_valid_terrain_peering_bit` :484.
- Custom data layers: `add_custom_data_layer` :500, `get_custom_data_layer_by_name` :503, `set_custom_data_layer_name` :504, `set_custom_data_layer_type(layer, Variant::Type)` :507. Any Variant type works, so passability, flags, region IDs and terrain tags would all go here.
- Physics layers :452-461 (`set_physics_layer_collision_layer`, `_physics_material`). Navigation :489-492. Occlusion :441-444.
- Tile proxies :511-521. Patterns `add_pattern` :536. `TileShape` / `TileLayout` / `TileOffsetAxis` :246-266.

**TileData** (class at :846)
- `set_custom_data(layer_name, value)` / `get_custom_data(layer_name)` :1028-1029 (bound .cpp:7176-7177). Also `has_custom_data`, `*_by_layer_id` :1030-1032.
- Terrain: `set_terrain_set` / `set_terrain` / `set_terrain_peering_bit` / `get_terrain_peering_bit` :1007-1013.
- `set_z_index` / `get_z_index` :969-970, `set_y_sort_origin` :971-972, `set_probability` / `get_probability` :1024-1025.
- Also flip_h, transpose, texture_origin, material, modulate :956-967, collision polygons with one-way :992-1001, navigation :1019, occluders :975-983.

**Animated tiles: exist** (on `TileSetAtlasSource`, :621)
- `TileAnimationMode` DEFAULT / RANDOM_START_TIMES :625-628.
- Per tile: `set_tile_animation_columns`, `_separation`, `_speed`, `_mode`, `_frames_count`, `_frame_duration`, `get_tile_animation_total_duration` :754-766.
- Flip and transpose are encoded as alternative-ID bits: `TRANSFORM_FLIP_H/V/TRANSPOSE` = 1<<12, 1<<13, 1<<14 (:632-634).

**Scene tiles: exist.** `TileSetScenesCollectionSource` :796, `create_scene_tile` :836, `set_scene_tile_scene` :838, `set_scene_tile_display_placeholder` :840.

**TileMap (old multi-layer node): deprecated.**
- Its doc says "Use multiple [TileMapLayer] nodes instead" (`doc/classes/TileMap.xml:2`).
- It is only registered inside `#ifndef DISABLE_DEPRECATED` (`scene/register_scene_types.cpp:851-855`).

**No quarter-tile autotile composition.** All "autotile" fields in tile_set.h (:155-184) belong to the Godot 3 compatibility block (`#ifndef DISABLE_DEPRECATED` at :152). You would have to pre-bake RPG Maker A1–A4 sheets into full tiles (the 47-tile blob = MATCH_CORNERS_AND_SIDES). There is no region-ID concept either (grep for "region_id|regions" in tile_set.h: 0 hits), so use a custom data layer.

**Editor (all in `editor/scene/2d/tiles/`)**
- Files: `tiles_editor_plugin.h` (TileMapEditorPlugin :121, TileSetEditorPlugin :156), `tile_map_layer_editor.h/.cpp`, `tile_set_editor`, `tile_set_atlas_source_editor`, `tile_set_scenes_collection_source_editor`, `tile_data_editors.h` (TileDataTerrainsEditor :336).
- Terrain painting: `TileMapLayerEditorTerrainsPlugin` (tile_map_layer_editor.h:274). `_draw_terrain_path_or_connect` (.cpp:2524) calls `terrain_fill_connect/path` (:2537-2539); `_draw_terrain_pattern` is at :2576. The "Paint terrain" undo actions are at :2903-2924.
- **Not extensible.** The sub-editors (`TileMapLayerSubEditorPlugin`, .h:67, with C++-only virtuals :82-91) are hard-coded in the constructor via `tile_map_editor_plugins.push_back(memnew(...))` (.cpp:4532-4533). There is no GDVIRTUAL or registration API.
- The only plugin points are the generic `EditorPlugin._forward_canvas_gui_input` / `_forward_canvas_draw_over_viewport` (`editor/plugins/editor_plugin.h:122-124`) and the runtime virtuals on TileMapLayer.

## 2. RESOURCES

**Resource** (`core/io/resource.h:52`)
- `duplicate(deep=false)` :154 and `duplicate_deep(mode=RESOURCE_DEEP_DUPLICATE_INTERNAL)` :155, bound at `core/io/resource.cpp:758-759`.
- Modes NONE / INTERNAL / ALL are in `core/variant/variant_deep_duplicate.h:36-40`, exposed as `Resource.DEEP_DUPLICATE_*` (resource.cpp:766-768).
- Per-property duplicate control: `PROPERTY_USAGE_ALWAYS_DUPLICATE` / `NEVER_DUPLICATE` (`core/object/property_info.h:108-109`).
- Local to scene: `set_local_to_scene` / `setup_local_to_scene` :161-163. Properties `resource_local_to_scene`, `resource_path`, `resource_name` (resource.cpp:774-776).
- Overridable: `_setup_local_to_scene`, `_get_rid`, `_reset_state`, `_set_path_cache` (:117-122). Signals `changed` and `setup_local_to_scene_requested` (resource.cpp:770-771). `emit_changed` :137, `copy_from` :134.

**ResourceSaver / ResourceFormatSaver** (`core/io/resource_saver.h`)
- `save(resource, path, flags)` :86.
- Flags :75-83: RELATIVE_PATHS, BUNDLE_RESOURCES, CHANGE_PATH, OMIT_EDITOR_PROPERTIES, SAVE_BIG_ENDIAN, COMPRESS, REPLACE_SUBRESOURCE_PATHS.
- Script virtuals: `_save`, `_set_uid`, `_recognize`, `_get_recognized_extensions`, `_recognize_path` :42-46.
- `add_resource_format_saver` :88. Script savers are auto-registered via `add_custom_savers` :101-103.

**ResourceLoader / ResourceFormatLoader** (`core/io/resource_loader.h`)
- `load(path, type_hint, cache_mode)` :259.
- Threaded: `load_threaded_request` / `_get_status` / `_get` :249-251.
- Cache modes IGNORE, REUSE, REPLACE, IGNORE_DEEP, REPLACE_DEEP :115-119.
- Script loader virtuals :62-73; `_load` is required (:73).
- Any global class (`class_name`) that extends ResourceFormatLoader is registered automatically (`resource_loader.cpp:1684-1697`).

**How modules register their formats**
- GDScript: `modules/gdscript/register_types.cpp:145-149` (`resource_loader_gd.instantiate(); ResourceLoader::add_resource_format_loader(...)`), removed at :189-192.
- Binary: `core/register_core_types.cpp:165-167`. JSON-as-resource: :253-257.
- Text: `scene/register_scene_types.cpp:377-378`.

**Text format (.tres / .tscn)** (`scene/resources/resource_format_text.cpp`)
- `FORMAT_VERSION = 4` (resource_format_text.h:46).
- Header is `[gd_resource type=... format=N uid="uid://..."]` (.cpp:1801-1815). `[ext_resource ... uid=...]` lines at :1882-1886, `[sub_resource` at :1923. Extensions at :1495-1496.

**Binary format (.res)** (`core/io/resource_format_binary.cpp`)
- `FORMAT_VERSION = 6` (:99). Magic "RSRC" (:2136, :2338). The compressed variant uses "RSCC" (:2110). Extension "res" (:2479-2480).

**ResourceUID** (`core/io/resource_uid.h:41`)
- `ID` is int64 (:44), `INVALID_ID = -1` (:45).
- `id_to_text` / `text_to_id` :72-73, `create_id` :75, `create_id_for_path` :76, `add_id` / `set_id` / `get_id_path` :78-80.
- Static `uid_to_path` / `path_to_uid` / `ensure_path` :84-86.

**Declaring properties** (`core/object/object.h`)
- Macros: `ADD_PROPERTY` :47, `ADD_GROUP` :50, `ADD_SUBGROUP` :52, `ADD_ARRAY_COUNT` :62.
- `MAKE_RESOURCE_TYPE_HINT("Font")` for typed resource arrays (:66-68).
- `PropertyInfo(type, name, hint, hint_string, usage, class_name)` is at `core/object/property_info.h:146`.
- Hints (:39-85) include RESOURCE_TYPE (with `-Excluded` subclass syntax, :57), ARRAY_TYPE (:71), LOCALIZABLE_STRING (:73), DICTIONARY_TYPE (:78), TOOL_BUTTON (:79), INPUT_NAME (:83), FILE_PATH (:84).
- Typed-collection hint strings: an Array uses the element type identifier; a Dictionary uses `"K;V"` (`core/variant/type_info.h:346-362`).

## 3. INSPECTOR & UNDO

**EditorInspectorPlugin** (`editor/inspector/editor_inspector.h:339`)
- Virtuals: `_can_handle`, `_parse_begin`, `_parse_category`, `_parse_group`, `_parse_property(object, type, name, hint, hint_string, usage, wide) -> bool`, `_parse_end` (:356-361; bound at `editor_inspector.cpp:1812-1817`).
- Helpers: `add_custom_control(control)` :368, `add_property_editor(property, editor, add_to_end=false, label="")` :369, `add_property_editor_for_multiple_properties` :370 (bound at .cpp:1808-1810).
- Register with `EditorPlugin.add_inspector_plugin` (`editor/plugins/editor_plugin.h:249`).

**EditorProperty** (.h:70)
- Virtuals `_update_property` and `_set_read_only` (:213-214).
- `emit_changed(property, value, field, changing)` :239 (bound .cpp:1634). `get_edited_object` / `get_edited_property` :257-258. `set_label` :245, `add_focusable` :290, `set_bottom_editor` :301. Signal `property_changed` (.cpp:1649).
- `EditorInspector.instantiate_property_editor` is a static that is bound for scripts (.cpp:3829, :6232).

**EditorUndoRedoManager** (`editor/editor_undo_redo_manager.h:36`)
- `create_action(name, merge_mode, custom_context, backward_undo_ops, mark_unsaved=true)` :92. `create_action_for_history` :91.
- `add_do_method` / `add_undo_method` (variadic) :97-116, bound as vararg methods at .cpp:548-561.
- `add_do_property` / `add_undo_property` :122-123, `add_do_reference` / `add_undo_reference` :124-125, `commit_action(execute=true)` :127.
- History IDs: GLOBAL_HISTORY = 0, REMOTE = -9, INVALID = -99 (:44-47). Signals `history_changed` and `version_changed` (.cpp:573-574).
- Get it from `EditorPlugin.get_undo_redo()` (editor_plugin.h:117) or `EditorInterface.get_editor_undo_redo()` (`editor/editor_interface.h:109`).
- Other plugin points:
  - `add_dock` (editor_plugin.h:167) and `add_control_to_bottom_panel` (:157).
  - `_has_main_screen` (:130) and `_handles` / `_edit` (:132-133).
  - `add_import_plugin` (:237).
  - `EditorInterface.edit_resource` / `inspect_object` (editor_interface.h:169-171).

## 4. EXPRESSION

- Signatures (`core/math/expression.h`):
  ```cpp
  Error parse(const String &p_expression, const Vector<String> &p_input_names = Vector<String>()); // :253
  Variant execute(const Array &p_inputs = Array(), Object *p_base = nullptr, bool p_show_error = true, bool p_const_calls_only = false); // :254
  ```
  Also `has_execute_failed` and `get_error_text` (:255-256). Bound at expression.cpp:1519-1522.
- **Calling methods on inputs and base_instance: yes.**
  - `a.method(...)` parses to a CallNode on the input (expression.cpp:887-903). It runs `base.callp(...)` (:1454).
  - A bare `foo()` becomes a call on `self`, i.e. the base_instance (:663-674).
  - A bare identifier that is not an input name becomes a named index on `self` (:718-723).
  - Property reads `a.atk` use `get_named` (:1321). So `"a.atk * 4 - b.def * 2"` with `parse(expr, ["a","b"])` and `execute([actor, target])` works.
- Language:
  - Keywords: `in`, `null`, `true`, `false`, `PI`, `TAU`, `INF`, `NAN`, `not`, `or`, `and`, `self` (:443-474). Built-in type constructors and all Variant utility functions are available (:476-487).
  - **Ternary / `if` does not exist** (no keyword at :443-474). **Assignment does not exist** (no assign token in the expression.h:47-86 token list).
- **Sandboxing is weak:**
  - `const_calls_only` switches to `Variant::call_const` (:1451-1452). That uses `Object::call_const`, which rejects only non-const *native* methods (`core/object/object.cpp:864-869`) and `free` (:834-837).
  - For script instances it forwards to `ScriptInstance::call_const`, and the default implementation just calls `callp(...)` (`core/object/script_instance.cpp:57-58`). So **every GDScript method is callable even with const_calls_only=true**. (GDScript has no override, per the grep of gdscript.h.)
  - Property reads (:1321) and utility functions (:1398-1417) are not gated at all. Utility functions include `print`, `str_to_var`, `bytes_to_var_with_objects` and `instance_from_id` (`core/variant/variant_utility.cpp:1753-1774`).
  - Formulas therefore need to be treated as trusted content, or wrapped in proxy objects that expose only safe getters.

## 5. LOCALIZATION

**TranslationServer** (`core/string/translation_server.h:36`)
- `set_locale` :110, `set_fallback_locale` :112, `add_translation` / `remove_translation` :140-141, `translate(msg, ctx)` :143, `translate_plural` :144.
- Pseudolocalization :148-150. `format_number` :152. `get_or_add_domain` :163.
- Project settings: `internationalization/locale/translations` (translation_server.cpp:762) and `.../locale/fallback` default "en" (:603).
- **TranslationDomain** (`core/string/translation_domain.h:37`) supports per-domain translations and `set_locale_override` (:97).

**Translation** (`core/string/translation.h:38`)
- `add_message` / `add_plural_message` / `get_message` / `get_plural_message` with context :84-87.
- Script-overridable `_get_message` / `_get_plural_message` :77-78.

**CSV import** (`editor/import/resource_importer_csv_translation.cpp`)
- Extension `csv` :47.
- Options: `compress`, `delimiter` (Comma / Semicolon / Tab), `unescape_keys`, `unescape_translations` (:71-74).
- Special columns `?context` (:104) and `?plural` (:109), and a special row `?pluralrule` (:150).
- PO/MO loading: `core/io/translation_loader_po.cpp:362-363`.

**POT generation**
- `TranslationTemplateGenerator` (`editor/translations/template_generator.h:36`). `generate()` writes `.pot` or `.csv` depending on the extension (template_generator.cpp:120-124).
- Its input is `internationalization/locale/translations_pot_files` (:107). `translation_add_builtin_strings_to_pot` controls built-in strings (:108).
- **EditorTranslationParserPlugin** (`editor/translations/editor_translation_parser.h:37`) has virtuals `_parse_file(path)` returning `TypedArray<PackedStringArray>`, `_get_recognized_extensions`, `_customize_strings` (:43-45).
  - Register with `EditorPlugin.add_translation_parser_plugin` (editor_plugin.h:234).
  - Built-in parsers: `modules/gdscript/editor/gdscript_translation_parser_plugin.h` and `editor/translations/packed_scene_translation_parser_plugin.h`. A custom parser could extract strings from your database `.tres` files.

**tr() and auto-translate**
- `Object::tr` / `tr_n` are at `core/object/object.h:810-811`. Per-object `set_translation_domain` is at :819.
- `auto_translate_mode` lives on **Node**, not Control:
  - `AutoTranslateMode` INHERIT / ALWAYS / DISABLED (`scene/main/node.h:130-133`), `set_auto_translate_mode` (:826), property at node.cpp:4059.
  - `atr` / `atr_n` (node.h:834-838) and `NOTIFICATION_TRANSLATION_CHANGED = 2010` (:498).
  - `Control::set_auto_translate` is deprecated (`scene/gui/control.h:851-854`). Control also has `tooltip_auto_translate_mode` (:857) and `set_layout_direction` for RTL (:845).

## 6. INPUT

**InputMap** (`core/input/input_map.h:40`)
- `DEFAULT_DEADZONE = 0.2` (:55), `DEFAULT_TOGGLE_DEADZONE = 0.5` (:57).
- `add_action` :83, `action_add_event` :90, `action_erase_event(s)` :92-93, `event_is_action` :96, `load_from_project_settings` :101.
- Actions are read from settings with the `input/` prefix, as `{deadzone, events}` (input_map.cpp:325-353).
- Built-in actions are registered as `input/<name>` with deadzone 0.5 (`core/config/project_settings.cpp:1648-1667`).

**Default ui_* actions** (input_map.cpp:452 onward)

| Action | Line | Default bindings |
|---|---|---|
| `ui_accept` | :462 | Enter, KP Enter, Space |
| `ui_select` | :467 | Joypad **Y**, Space |
| `ui_cancel` | :471 | Escape |
| `ui_close_dialog` | :475 | Escape |
| `ui_focus_next` / `ui_focus_prev` | :484 / :488 | Tab / Shift+Tab |
| `ui_left` / `ui_right` / `ui_up` / `ui_down` | :494 / :500 / :506 / :512 | Arrow key + D-pad + left stick |
| `ui_page_up` / `ui_page_down` / `ui_home` / `ui_end` | :516-528 | PageUp / PageDown / Home / End |
| `ui_menu` | :803 | Menu key |

- Editing and other actions also exist: `ui_cut`, `ui_copy`, `ui_paste`, `ui_undo`, `ui_redo`, `ui_text_*`, `ui_graph_*`, `ui_filedialog_*`, `ui_swap_input_direction`, `ui_colorpicker_delete_preset`, `ui_accessibility_drag_and_drop`, `ui_unicode_start`, `ui_focus_mode` (:531-881).
- **No gamepad binding for `ui_accept` or `ui_cancel`.** The only joypad defaults in the whole file are Y (:465), the D-pad and left stick (:492-511), and X for the color picker (:879). Map A/B yourself.

**Input singleton and events**
- `is_action_pressed` / `_just_pressed` / `_just_released` / `get_action_strength` / `get_axis` / `get_vector` (`core/input/input.h:361-370`).
- Joypads: `get_connected_joypads` :374, `joy_connection_changed` :381, `start_joy_vibration` :431, `add_joy_mapping` :461. The controller DB is in `core/input/gamecontrollerdb.txt`.
- `action_press` :440, `parse_input_event` :396.
- `InputEventAction` (`core/input/input_event.h:452`): `set_action`, `set_pressed`, `set_strength`, `set_event_index` (:463-471).

## 7. TEXT / UI

**RichTextLabel** (`scene/gui/rich_text_label.h:50`)
- Typewriter support: `set_visible_characters` :1008, `get_total_character_count` :1012, `set_visible_ratio` :1015, `visible_characters_behavior` :1018-1019. Properties at .cpp:8043-8045.
- **There is no built-in reveal speed, auto-advance or wait/pause tag.** No such tag is in the parser list, and the only "speed"/"wait" hits in the header are unrelated (`push_rainbow` speed, `wait_until_finished` for threaded loading). Drive `visible_characters` yourself with a Tween or `_process`.
- Methods: `append_text` :970, `parse_bbcode` :969, `push_meta` :848, `push_customfx(effect, env)` :859. Signals `meta_clicked` (.cpp:8053) and `finished` (:8057).
- BBCode tags, with parser lines in rich_text_label.cpp:
  - Text style: `b` 5669, `i` 5679, `u` 5812, `s` 5826, `code` 5689.
  - Layout: `center` 5898, `left` 5906, `right` 5910, `fill` 5902, `indent` 5943, `p` 5956, `br` 5952, `ul`/`ol` 5656, `table=` 5694, `cell` 5746.
  - Colour and font: `color=` 6368, `bgcolor=` 6729, `fgcolor=` 6737, `outline_color=` 6375, `outline_size=` 6586, `font=` 6447, `font_size=` 6382.
  - Inline content: `img` 6202, `url` 6057, `hint=` 6101, `dropcap` 6106, `lang=` 5947, `char=` 5840.
  - Built-in effects: `fade` 6592, `shake` 6608, `wave` 6631, `tornado` 6654, `rainbow` 6677, `pulse` 6706.
- **Custom effects:**
  - Add them through the `custom_effects` property, an Array of RichTextEffect (.cpp:8028), or `install_effect` (.h:1024).
  - A tag is matched against the effect's `bbcode` script variable (rich_text_label.cpp:8401-8413; `rich_text_effect.cpp:47-50`).
  - Override `RichTextEffect._process_custom_fx(CharFXTransform)` (`scene/gui/rich_text_effect.h:107`).
  - CharFXTransform exposes range, elapsed_time, visible, outline, offset, color, environment, glyph_index, relative_index, font (:36-97).

**Window skins**
- `NinePatchRect` (`scene/gui/nine_patch_rect.h:35`): `AxisStretchMode` STRETCH / TILE / TILE_FIT (:39-42), `set_patch_margin` :64, `set_region_rect` :67, `set_draw_center` :70.
- `StyleBoxTexture` (`scene/resources/style_box_texture.h:36`): texture_margin :64-66, expand_margin :69-71, region_rect :74, the same stretch modes (:40-43), modulate :86.
- `Theme` (`scene/resources/theme.h:38`): DataType COLOR / CONSTANT / FONT / FONT_SIZE / ICON / STYLEBOX (:56-63), `set_stylebox` :145, `set_type_variation` :219.

**Focus navigation**
- Control: `FocusMode` NONE / CLICK / ALL / ACCESSIBILITY (`control.h:66-70`), `set_focus_mode` :719, `grab_focus` :725, `find_valid_focus_neighbor` :731, `set_focus_neighbor(side, path)` :733, `set_focus_next/previous` :736-738, `_gui_input` virtual :489, `set_theme` / `set_theme_type_variation` :790-793.
- The Viewport moves focus on `ui_up/down/left/right/focus_next/focus_prev` (`scene/main/viewport.cpp:2344-2405`). Buttons fire on `ui_accept` (`scene/gui/base_button.cpp:96`).

## 8. 2D CHARACTERS & ANIMATION

- **AnimatedSprite2D** (`scene/2d/animated_sprite_2d.h:36`): `play(name, custom_scale, from_end)` :93, `play_backwards` :94, `pause` / `stop` :95-96, `set_frame_and_progress` :112, `set_speed_scale` :114, `set_flip_h` :124. Signals `animation_finished`, `animation_looped`, `frame_changed`, etc. (.cpp:685-689).
- **SpriteFrames** (`scene/resources/sprite_frames.h:37`): `add_animation` :68, `set_animation_speed(fps)` :77, `set_animation_loop` :81, `set_animation_loop_mode` :85, `add_frame(anim, tex, duration, at)` :88.
- **Sprite2D** sheets (`scene/2d/sprite_2d.h`): `set_frame` :123, `set_frame_coords` :126, `set_vframes` / `set_hframes` :129-132. This fits RPG Maker character sheets.
- **AnimationPlayer** (`scene/animation/animation_player.h:36`): `play` :205, `play_section(_with_markers)` :206-207, `play_with_capture` :211, `queue` :212, `stop(keep_state)` :216, `seek` :235. Signals `animation_finished` / `animation_started` (`animation_mixer.cpp:2536-2537`). Track types VALUE / METHOD / BEZIER / AUDIO / ANIMATION (`scene/resources/animation.h:48-57`). Method and audio tracks fit battle animations.
- **Tween** (`scene/animation/tween.h:67`):
  - `tween_property` :148, `tween_interval` :149, `tween_callback` :150, `tween_method` :151, `tween_subtween` :152, `tween_await(signal)` :153 (AwaitTweener with `set_timeout`, :334-344).
  - `set_parallel` :175, `set_loops` :176, `set_trans` / `set_ease` :179-181. TransitionType :84-97, EaseType :100-105.
  - Created with `Node.create_tween` (`scene/main/node.h:621`) or `SceneTree.create_tween` (`scene_tree.h:434`).
- **Camera2D** (`scene/2d/camera_2d.h:35`): `set_limit_rect` :128, `set_offset` :131, `set_limit_enabled` :140, `set_limit(side, px)` :143, `set_limit_smoothing_enabled` :146, drag margins :149-161, `set_position_smoothing_enabled/speed` :164-167, `set_zoom` :186, `reset_smoothing` :197, `align` :198. **There is no shake API** (0 hits for "shake"); tween `offset` instead.
- **CharacterBody2D** (`scene/2d/physics/character_body_2d.h:36`): `move_and_slide` :49, `MotionMode` GROUNDED / FLOATING :40-42 (FLOATING suits top-down). `PhysicsBody2D.test_move` (`physics_body_2d.h:52`).
- For grid movement without physics:
  - Use TileMapLayer `local_to_map` / `map_to_local` with tile custom data.
  - **AStarGrid2D** (`core/math/a_star_grid_2d.h:37`): `set_region` :179, `set_point_solid` :212, `get_id_path` :226, `DIAGONAL_MODE_NEVER` :43.

## 9. AUDIO

- **AudioStreamPlayer** (`scene/audio/audio_stream_player.h:41`):
  - Playback: `play(from_pos)` :91, `seek` :92, `get_playback_position` :95. Memorising and resuming BGM is possible with these.
  - Settings: `set_volume_db` :79, `set_volume_linear` :82, `set_pitch_scale` :85 (range hint "0.01,4,...,or_greater" at .cpp:286), `set_max_polyphony` :88, `set_bus` :97, `set_stream_paused` :106.
  - `get_stream_playback` :110. Signal `finished` (.cpp:295).
- **AudioServer** (`servers/audio/audio_server.h:180`): `add_bus` :385, `set_bus_name` :389, `get_bus_index` :391, `set_bus_volume_db/linear` :395-398, `set_bus_send` :401, `set_bus_mute` :407, `add_bus_effect` :413, `set_playback_speed_scale` :430, `set_bus_layout` :488. Effects are in `servers/audio/effects/` (reverb, filter, pitch_shift, delay, eq, compressor, limiter, chorus).
- **Formats and looping:**
  - Ogg Vorbis (`modules/vorbis/audio_stream_ogg_vorbis.h:115`): `set_loop` :145, `set_loop_offset(seconds)` :148, `bpm` / `beat_count` / `bar_beats` :151-157, `load_from_file` :142.
  - MP3 (`modules/mp3/audio_stream_mp3.h:90`): `set_loop` :118, `set_loop_offset` :121.
  - WAV (`scene/resources/audio_stream_wav.h:95`): LoopMode DISABLED / FORWARD / PINGPONG / BACKWARD :108-112, `loop_begin/end` (frames) :142-145.
- **Interactive music: exists.** Module `modules/interactive_music` always builds (`config.py:1-2`).
  - `AudioStreamInteractive` (`audio_stream_interactive.h:37`): transition from IMMEDIATE / NEXT_BEAT / NEXT_BAR / END and to SAME_POSITION / START / PREVIOUS_POSITION, fades, auto-advance (:41-72), `add_transition` :168. The playback object has `switch_to_clip(_by_name)` :261-262.
  - `AudioStreamPlaylist` (`audio_stream_playlist.h:37`): up to 64 streams, fade_time, shuffle, loop (:45-66).
  - `AudioStreamSynchronized` (:37, up to 32 layers): useful for layered BGS.
- **AudioStreamRandomizer** (`servers/audio/audio_stream.h:272`) gives SE variation: random pitch and volume offset (:327-333). `AudioStreamPolyphonic` is in `scene/resources/audio_stream_polyphonic.h`.

## 10. SAVE / PERSISTENCE

- **FileAccess** (`core/io/file_access.h:45`):
  - Modes READ / WRITE / READ_WRITE / WRITE_READ :57-61.
  - `open` :242, `open_encrypted(path, mode, key, iv)` :245, `open_encrypted_pass` :246. The last is **bound as `open_encrypted_with_pass`** (file_access.cpp:1018).
  - `open_compressed` :247, with FASTLZ / DEFLATE / ZSTD / GZIP / BROTLI (:80-85).
  - `store_var(value, full_objects=false)` :232 and `get_var(allow_objects=false)` :189. `get_file_as_string` / `_bytes` :277-278.
  - Encryption is AES-256 (`core/io/file_access_encrypted.h:42-45`) with an MD5 integrity check (`file_access_encrypted.cpp:111`).
- **JSON** (`core/io/json.h:36`): `stringify(var, indent, sort_keys=true, full_precision=false)` :90, `parse_string` :91, `parse` :87, `from_native` / `to_native` (Variant <-> JSON-safe, optional objects) :93-97. JSON is also a Resource with its own loader/saver (register_core_types.cpp:253-257).
- **ConfigFile** (`core/io/config_file.h:38`): `set_value` / `get_value` :52-53, `save` / `load` :64-65, `load_encrypted(_pass)` / `save_encrypted(_pass)` :72-76.
- **var_to_str / str_to_var / var_to_bytes / bytes_to_var / *_with_objects** are at `core/variant/variant_utility.h:142-147` (bound .cpp:1763-1770).
- **user://**: `OS::get_user_data_dir` (`core/os/os.h:326-327`); `application/config/use_custom_user_dir` and `custom_user_dir_name` (`project_settings.cpp:1702-1703`). The exact `user://` path-resolution code: not verified.

## 11. PARTICLES / SCREEN EFFECTS

- **CanvasModulate** (`scene/2d/canvas_modulate.h:35`, `set_color` :53) gives a whole-canvas tint, i.e. RPG Maker "Tint Screen".
- **CanvasLayer** (`scene/main/canvas_layer.h:37`): `set_layer` :71, `set_transform` :79, `set_offset` :83, `set_follow_viewport` :102. Use it for UI or overlays above the map.
- **CanvasItem** (`scene/main/canvas_item.h`): `set_modulate` :306, `set_self_modulate` :310 (sprite flash/tint), `set_material` :406, `set_instance_shader_parameter` :409, `set_use_parent_material` :412, `set_clip_children_mode` :300.
- **Screen flash:** use a ColorRect (`scene/gui/color_rect.h`) on a CanvasLayer and tween its alpha. **There is no built-in flash, shake or tint helper.**
- **Shaders:** `hint_screen_texture` (`servers/rendering/shader_language.cpp:397`), `SCREEN_UV` in canvas_item fragment shaders (`servers/rendering/shader_types.cpp:306`). `BackBufferCopy` (`scene/2d/back_buffer_copy.h:35`, COPY_MODE_RECT / VIEWPORT :39-42) supports screen-space post effects such as wave or blur.
- **GPUParticles2D** (`scene/2d/gpu_particles_2d.h:35`): `set_emitting` :115, `set_amount` :116, `set_lifetime` :117, `set_one_shot` :118, `set_explosiveness_ratio` :120, `set_process_material` :124 (ParticleProcessMaterial in `scene/resources/particle_process_material.h`), `set_sub_emitter` :177, `emit_particle` :194, `restart(keep_seed)` :196. Signal `finished` (.cpp:951).
- **CPUParticles2D** (`scene/2d/cpu_particles_2d.h:39`): `restart` :330, `convert_from_particles` :332.
