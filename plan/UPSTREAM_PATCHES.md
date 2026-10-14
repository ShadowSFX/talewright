# Upstream (core) patches ledger

Talewright adds new modules and directories instead of patching Godot. Any change to a file that exists
upstream (`git diff 4.7.2-stable -- <path>` non-empty outside `modules/jrpg`, `modules/mmo`,
`modules/claude_bridge`, `talewright/`, `plan/`, `CLAUDE.md`, `TALEWRIGHT.md`) must be listed here so
merging later upstream releases stays easy.

Check: `python plan/tools/core_diff_check.py` (fails on any changed upstream path not listed in the table below).

| # | File(s) | Change | Reason (why unavoidable) | Task | Upstreamable? |
|---|---------|--------|--------------------------|------|---------------|
| 1 | `.pre-commit-config.yaml` | `copyright-headers` hook excludes `modules/(claude_bridge\|jrpg\|mmo)/` | Godot's hook rewrites every C++ header to "This file is part of: GODOT ENGINE" with Godot's copyright; fork-authored files carry the Talewright header (D-027); `talewright/tools/add_header.py --check` checks them instead | 5.1.2 | No (fork-only) |
| 2 | `.github/CODEOWNERS` | Owner entries for the fork paths (`/plan/`, `/talewright/`, fork modules, `CLAUDE.md`, `TALEWRIGHT.md`) at the end of the file | The `validate-codeowners` hook fails CI for files without an owner (D-027) | 5.1.2 | No (fork-only) |
| 3 | `doc/classes/ProjectSettings.xml` | Member entry for `jrpg/enabled` | CI's doc-test runs `--doctool` and fails when a registered setting is undocumented; `jrpg/enabled` is the module's only project setting (D-008) | 5.2.1 | No (fork-only) |
| 4 | `.github/actions/godot-cpp-build/action.yml` | Moves the godot-cpp checkout to `$RUNNER_TEMP` before the SCons build | godot-cpp archives ~1100 objects with one `ar` command whose paths repeat the workspace path; `/home/runner/work/talewright/talewright/` made it 131 524 bytes, over Linux's 131 072-byte single-argument limit ("Argument list too long", CI run 38489070426). The temp path is shorter than upstream's (D-028) | CI | No (fork-only) |
| 5 | `.github/workflows/linux_builds.yml` | The "doubles and GCC sanitizers" job adds `module_claude_bridge_enabled=no module_jrpg_enabled=no module_mmo_enabled=no` | With the fork modules the unoptimized ASan+UBSan binary passes 2 GiB and mold fails (`.eh_frame` relocation out of range by 512 KiB, same run). The clang ASan/UBSan and ThreadSanitizer jobs keep building and testing the fork modules (D-028) | CI | No (fork-only) |

## Candidates observed upstream (not patched; only if they start to matter)
- `thirdparty/enet/enet_godot.cpp:366` — `peers.size() < max_clients && status == HANDSHAKING || status == CONNECTED`
  operator precedence: the DTLS client cap is not applied on the CONNECTED branch. Talewright servers
  enforce their own connection caps, so no patch planned.
- `modules/enet/enet_multiplayer_peer.cpp:67` — `create_host_bound` arguments appear shifted by one
  (in-bandwidth ignored, channel limit 0). Talewright uses `ENetConnection` directly, so not affected.
