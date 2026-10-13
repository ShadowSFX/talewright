# Review — Epics 0 (bootstrap) and 1 (toolchain baseline)

Reviewer: fresh-context subagent, 2026-10-13. Scope: plan/tools/*, talewright/tools/*, samples, CLAUDE.md,
TALEWRIGHT.md, DECISIONS.md, UPSTREAM_PATCHES.md. Confirmed OK by the reviewer: validate ok, ROADMAP render
byte-identical, no core Godot files modified, commits prefixed with task ids and free of Co-Authored-By trailers.
Fixes landed in task 1.6.

| # | Sev | Finding | Fix |
|---|---|---|---|
| 1 | high | `tasks.py import` reset status/commit/notes of existing ids | `cmd_import` updates only the fields present in the file; defaults only for new ids. Verified: re-importing 2.1.x `files` kept `in_progress`. |
| 2 | high | Resume protocol gaps (parallel in-progress tasks, done-but-uncommitted, stamp-only dirty tree; `next` printed nothing) | New `tasks.py doctor`; `next` lists in-progress tasks, ready tasks and unsplit epics and explains "nothing ready"; CLAUDE.md step 2 covers all three cases; research tasks now record their output files. |
| 3 | high | `HEADER_RE.sub` crashed on backslashes; files written before PROGRESS → desync | Lambda replacement; `write_all` computes PROGRESS text and validates before writing any file. |
| 4 | medium | Containers never reopened | `sync_containers` recomputes container status both ways after every change (verified: adding 1.6 reopened epic 1). |
| 5 | medium | Epics 0–3 had no epic-review child | Added 2.5 and 3.5; this review recorded under 1.6 (epics 0 and 1). |
| 6 | medium | validate missed ancestor/descendant deps and cycles through containers | Rejects deps on own ancestors/descendants; cycle detection on the leaf graph with container deps expanded to leaves. |
| 7 | medium | build.py printed "errors: 0" for non-compiler failures | Prints the last 30 log lines when exit ≠ 0 and no error line matched. |
| 8 | medium | Docs showed a direct SCons line that fails on this machine; shim needed the env var | site_init falls back to its own `vswhere_vc.cmd` (verified: plain `python -m SCons --site-dir=...` finds VS 14.3); CLAUDE.md documents only the wrapper and what it adds. |
| 9 | medium/low | Deferred work satisfied dependencies; all-deferred containers became "done" | Deps require `done`; containers become `deferred` when all children are deferred. |
| 10 | low | Childless epics could be started as one task | `start` refuses top-level tasks without subtasks; `next` lists them as "split first". |
| 11 | low | smoke ERROR regex wrong (`USER ERROR`, unreachable `at:` pattern, missed `SHADER ERROR`); exceptions instead of FAIL; PASS without exported exe | Regex `^\s*(ERROR|SCRIPT ERROR|SHADER ERROR):` (verified to catch `push_error` output); `run()` handles timeouts/OSError; `check(..., must_exist=)`; UTF-8-safe output; full per-run log file. |
| 12 | low | `next`/`show` crashed on non-cp1252 output | `sys.stdout.reconfigure(encoding="utf-8", errors="replace")`. |
| 13 | low | Kill matched `bin_old\…` and killed every bin/ process | Kill only `bin\godot.windows.<target>*` for the target being built. |
| 14 | low | check_citations split on `\f` etc.; zero citations exited 0; crawler silently returned [] and had no visited set | `split("\n")`; zero citations → exit 1; crawler raises on format change/empty TOC and tracks visited keys (re-crawl identical to committed TOC). |

Also noted by the reviewer: an undiagnosed single smoke failure earlier in the session (output lost); the smoke
runner now keeps full logs (`talewright/build_logs/smoke-*.log`) so any recurrence can be diagnosed; 9
consecutive runs since then passed.
