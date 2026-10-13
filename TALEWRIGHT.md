# Talewright

**Talewright is a JRPG-focused fork of the Godot Engine.** It keeps every Godot feature, workflow and
project type working unchanged and adds optional modules:

- **JRPG workspace** (`modules/jrpg`): a single integrated place in the editor to build JRPGs — database,
  map editor, events, messages, menus, battles, save/load — reimplemented from public documentation of
  classic JRPG makers; projects can use it or ignore it.
- **MMO tooling** (`modules/mmo`): login/world/zone server programs built from the same engine so a JRPG
  or ARPG made in the workspace can run online.
- **Claude bridge** (`modules/claude_bridge`): an editor-only MCP server (localhost, token-protected) that
  lets Claude Code inspect and drive the editor, the running game and the server cluster. It is never
  compiled into export templates.

Building the engine with `module_jrpg_enabled=no module_mmo_enabled=no module_claude_bridge_enabled=no`
yields vanilla Godot.

## Trademarks and affiliation

Talewright is not affiliated with or endorsed by the Godot Foundation. It uses the GODOT® name under the
Godot Foundation's permissive trademark license only to state that it is based on the Godot Engine
(https://godot.foundation/policies-and-procedures/trademark-policy). The Godot logo is not used as
Talewright's logo. Talewright is not affiliated with, endorsed by or derived from any RPG Maker product;
"RPG Maker" is a trademark of its respective owner. No RPG Maker code, assets, text or file formats are
included.

## License

The Godot Engine is MIT-licensed (`LICENSE.txt`, `COPYRIGHT.txt`); Talewright's additions are under the
same MIT license unless a file states otherwise. Third-party components added by Talewright and the
licenses of all default assets are listed in `talewright/assets/LICENSES.md` and the module `thirdparty`
notices.

## Where things are

- Plan, tracker and decisions: `plan/` (start with `plan/ROADMAP.md` and `plan/DECISIONS.md`).
- Agent instructions and the resume protocol: `CLAUDE.md`.
- Tools and samples: `talewright/`.
