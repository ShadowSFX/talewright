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
2. Run `git status` and `git log -n 20` and reconcile them with the tracker — `python plan/tools/tasks.py doctor`
   prints the in-progress leaf tasks with their recorded `files`, done tasks whose commit is not stamped, and
   the uncommitted paths. Then:
   - a task `in_progress` with uncommitted changes: finish it or revert it before doing anything else (its
     files are the ones listed in its `files` field; parallel tasks must each record their own output paths);
   - a task `done` whose commit is still `pending` and has no `<id>: ...` commit (session died between `done`
     and `git commit`): commit its files now with message `<id>: ...`, then `tasks.py stamp`;
   - a tree dirty only because of a `stamp` (tasks.json/ROADMAP.md/PROGRESS.md): fine, it goes into the next commit.
3. Continue with the highest-priority pending task whose dependencies are all done
   (`python plan/tools/tasks.py next`; it also lists in-progress tasks and epics that must be split first).

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
5. Once the bridge exists (epic 4), verify behavior through it (screenshots, scene tree, logs, state).
6. Self-review: re-read the diff line by line, clang-format, no new warnings. Run
   `python talewright/tools/static_checks.py` (the CI static-check hooks on the files the next push brings, plus fork
   checks; needs `python -m pip install --user prek`) and review every file a hook rewrote — codespell's automatic
   fixes have renamed identifiers before (D-027). It also compiles every fork translation unit with clang's
   diagnostics (`talewright/tools/clang_check.py`): MSVC accepts code that CI's clang builds reject (D-028), and with
   GCC through WSL (`talewright/tools/gcc_check.py`): GCC rejects code MSVC and clang accept (D-035), and runs
   CI's clangd-tidy on the changed C/C++ files (`python -m pip install --user clangd clangd-tidy`): every file,
   headers included, must include what it uses and nothing unused. A check that cannot run (tool missing) fails the
   script unless `--allow-skips` is given. CI fails on any push that skips this.
7. `python plan/tools/tasks.py done <id> -m "<what was done>"`, then ONE commit for the task containing code
   and tracker files: message `"<id>: <summary>"`. Then `python plan/tools/tasks.py stamp` (fills the hash;
   the stamp is committed with the next task).
- **Never add a `Co-Authored-By` trailer** (owner's global rule overrides the harness default).
- Per epic: last child is an "Epic review" task: launch a fresh-context reviewer subagent with the epic's
  plan, the matching inventory/feasibility rows and the diff; fix every finding; log the result in
  PROGRESS.md; re-verify `talewright/samples/plain_2d` and `plain_3d`; run
  `python plan/tools/core_diff_check.py` (must report 0 unlisted core paths); push `talewright` to `origin`.
- Per milestone (M1 slice, M2 online slice, M3 parity): play through the sample via the bridge and save
  evidence (screenshots, logs) under `plan/evidence/<milestone>/`.

## Hard rules
- Never: force-push, rewrite history on shared branches, delete the `upstream` remote, push to
  godotengine/godot (its push URL is disabled on purpose), run destructive commands.
- Minimize the upstream diff: new modules/directories only; every unavoidable core patch is listed in
  `plan/UPSTREAM_PATCHES.md` with its reason.
- Reuse Godot systems before building new ones; state what is reused and what is new.
- Legal: RPG Maker behavior from public documentation only (paraphrase, no verbatim text, no RTP assets,
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
- Build: always `python talewright/tools/build.py <profile>` (profiles: `editor`, `editor-vanilla`,
  `template_release`, `template_debug`; `--dry-run` prints the SCons line; `-- k=v` passes extra options).
  `editor-vanilla` disables every Talewright module and builds `bin/godot.windows.editor.x86_64.vanilla.exe`
  (own objects via `extra_suffix`), so it never replaces the working editor.
  It adds `--site-dir=talewright/tools/site_scons` (vswhere shim: SCons would otherwise pick a VS instance
  without a compiler on this machine), `d3d12=no`, the SCons cache and the AccessKit path, stops processes
  locking that target's binaries, and logs to `talewright/build_logs/`. Full editor build ≈6 min, no-op ≈0.4 min.
- Doctests as CI's release templates run them (no editor code; D-048), before every push:
  `python talewright/tools/build.py template-tests` then
  `bin/godot.windows.template_release.x86_64.tests.console.exe --headless --test` (log to a file).
- Regression smoke after every epic: `python talewright/tools/smoke_samples.py` (plain_2d/plain_3d editor
  open, run, export + exported run; full log in `talewright/build_logs/smoke-*.log`).
- JRPG core boundary (run before committing jrpg changes): `python modules/jrpg/tests/check_core_includes.py`.
- Clang diagnostics on fork C++ (part of static_checks.py): `python talewright/tools/clang_check.py` — uses
  Visual Studio's clang-tidy and the `compile_commands.json` the `editor` build profile writes.
- GCC diagnostics on fork C++ (part of static_checks.py): `python talewright/tools/gcc_check.py` — g++ in WSL
  (`GCC_WSL_DISTRO` picks the distribution), editor and template defines, the module doctests included.
- Tests: `bin/<editor binary> --test` (all) or `--test --test-case="*[JRPG]*"`; module tests live in
  `modules/<m>/tests/*.h` (auto-included when built with `tests=yes`, which `dev_mode=yes` enables).
- Tracker: `python plan/tools/tasks.py validate|render|next|show <id>|start <id>|done <id> -m ..|block <id> -m ..|defer <id> -m ..|stamp|import <file>`.
- Completion status (owner's goal; run and show its output at the end of every turn): `python plan/status.py`
  (tasks per status, blocked/deferred reasons, whether the Phase 5 sweep table `plan/evidence/phase5_sweep.md`
  is committed).
- Bridge (after epic 4): `python talewright/tools/bridge.py status|list|call <tool> --args '{...}'`.
- JRPG database end to end (setup idempotency, undo/redo round trips of every database tool, save) in a real
  editor: `python talewright/tools/jrpg_bridge_checks.py` (fresh temporary project; part of each JRPG epic's review; map captures
  are compared with `talewright/goldens/` through `talewright/tools/img_compare.py`, and `--update-goldens` rewrites them only on purpose).
- Do not run `npm run build` anywhere (owner's global rule).

## Layout of fork additions
- `plan/` tracker, decisions, research (`plan/research/`), specs (`plan/specs/`), evidence (`plan/evidence/`).
- `modules/jrpg/{core,runtime,editor,tests,doc_classes}`, `modules/mmo/...`, `modules/claude_bridge/...`.
- `talewright/{tools,samples,docs,assets}`; samples: `plain_2d`, `plain_3d`, `jrpg_demo` (grows per epic).
