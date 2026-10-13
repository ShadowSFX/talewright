# CLAUDE.md — Talewright

Talewright is a personal fork of the Godot Engine (base `4.7.2-stable`, branch `talewright`) that stays a
full general-purpose engine and adds, as optional new modules:
- `modules/jrpg` — a centralized JRPG workspace at RPG Maker MZ parity (database, maps, events, battles…),
- `modules/mmo` — MMO tooling (login/world/zone servers built from this engine, rAthena techniques),
- `modules/claude_bridge` — an editor-only MCP server so Claude Code can see and drive the editor, the
  running game and the server cluster.
Fork-only tools, samples and docs live in `talewright/`; the plan and tracker live in `plan/`.
Approved plan: `C:\Users\Relic\.claude\plans\zesty-hugging-wilkinson.md` (summary in `plan/DECISIONS.md`).

## Resume protocol (follow at the start of every session, after any context compaction, or when unsure)
1. Read `plan/PROGRESS.md`, `plan/tasks.json` and `plan/DECISIONS.md`.
2. Run `git status` and `git log -n 20` and reconcile them with the tracker. If a task is `in_progress`
   with uncommitted changes, finish it or revert it before doing anything else.
3. Continue with the highest-priority pending task whose dependencies are all done
   (`python plan/tools/tasks.py next`).

A new session given only "continue" must resume from these files alone. Never ask the owner anything:
when a decision is needed, choose, record it in `plan/DECISIONS.md` (context, options, choice, why,
reversal) and continue. Never idle: if a task is truly blocked, `tasks.py block <id> -m "<exact reason>"`
and move on to the next ready task.

## Per-task loop
1. `python plan/tools/tasks.py start <id>` (updates tasks.json, ROADMAP.md, PROGRESS.md).
   Split any task too large for one focused sitting into subtasks first (`tasks.py import <file.json>`).
2. Implement. Verify every Godot claim by reading the code and cite `file:line`; write "not verified" when
   you cannot verify; mark anything that does not exist yet as NEW.
3. Build (incremental). 4. Run upstream tests + the new doctests for the touched system.
5. Once the bridge exists (epic 4), verify behaviour through it (screenshots, scene tree, logs, state).
6. Self-review: re-read the diff line by line, clang-format, no new warnings.
7. `python plan/tools/tasks.py done <id> -m "<what was done>"`, then ONE commit for the task containing code
   and tracker files: message `"<id>: <summary>"`. Then `python plan/tools/tasks.py stamp` (fills the hash;
   the stamp is committed with the next task).
- **Never add a `Co-Authored-By` trailer** (owner's global rule overrides the harness default).
- Per epic: last child is an "Epic review" task: launch a fresh-context reviewer subagent with the epic's
  plan, the matching inventory/feasibility rows and the diff; fix every finding; log the result in
  PROGRESS.md; re-verify `talewright/samples/plain_2d` and `plain_3d`; push `talewright` to `origin`.
- Per milestone (M1 slice, M2 online slice, M3 parity): play through the sample via the bridge and save
  evidence (screenshots, logs) under `plan/evidence/<milestone>/`.

## Hard rules
- Never: force-push, rewrite history on shared branches, delete the `upstream` remote, push to
  godotengine/godot (its push URL is disabled on purpose), run destructive commands.
- Minimize the upstream diff: new modules/directories only; every unavoidable core patch is listed in
  `plan/UPSTREAM_PATCHES.md` with its reason.
- Reuse Godot systems before building new ones; state what is reused and what is new.
- Legal: RPG Maker behaviour from public documentation only (paraphrase, no verbatim text, no RTP assets,
  no file formats); never "RPG Maker" or bare "Godot" in names (Godot trademark policy, see
  `TALEWRIGHT.md`); default assets are original (generated) or clearly licensed with the license recorded;
  rAthena/GPL projects are references only — never copy their code. `C:\Users\Relic\Desktop\ro server` is
  read-only.
- UI: use the `frontend-design` skill for all new editor UI, after citing the Godot editor UI patterns being
  followed (EditorDock, main screen plugins, inspector/EditorProperty, bottom panel, editor theme/icons).
- Definition of done also requires: runtime features declare online authority (D-007); editor features
  expose an authoring API + bridge tool (D-012).

## Commands (Windows; Git Bash shown)
- SCons is installed per-user: `python -m SCons` (4.10.1). clang-format 21.1.7:
  `"$APPDATA/Python/Python311/Scripts/clang-format.exe" -i <files>` (repo `.clang-format`).
- Build (exact flags confirmed in task 1.2 and recorded in `plan/research/godot_current_state.md`):
  `python talewright/tools/build.py editor` (wrapper; created in task 1.1) or directly
  `python -m SCons platform=windows target=editor dev_mode=yes -j32`.
- Tests: `bin/<editor binary> --test` (all) or `--test --test-case="*[JRPG]*"`; module tests live in
  `modules/<m>/tests/*.h` (auto-included when built with `tests=yes`, which `dev_mode=yes` enables).
- Tracker: `python plan/tools/tasks.py validate|render|next|show <id>|start <id>|done <id> -m ..|block <id> -m ..|defer <id> -m ..|stamp|import <file>`.
- Bridge (after epic 4): `python talewright/tools/bridge.py status|list|call <tool> --args '{...}'`.
- Do not run `npm run build` anywhere (owner's global rule).

## Layout of fork additions
- `plan/` tracker, decisions, research (`plan/research/`), specs (`plan/specs/`), evidence (`plan/evidence/`).
- `modules/jrpg/{core,runtime,editor,tests,doc_classes}`, `modules/mmo/...`, `modules/claude_bridge/...`.
- `talewright/{tools,samples,docs,assets}`; samples: `plain_2d`, `plain_3d`, `jrpg_demo` (grows per epic).
