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

## Candidates observed upstream (not patched; only if they start to matter)
- `thirdparty/enet/enet_godot.cpp:366` — `peers.size() < max_clients && status == HANDSHAKING || status == CONNECTED`
  operator precedence: the DTLS client cap is not applied on the CONNECTED branch. Talewright servers
  enforce their own connection caps, so no patch planned.
- `modules/enet/enet_multiplayer_peer.cpp:67` — `create_host_bound` arguments appear shifted by one
  (in-bandwidth ignored, channel limit 0). Talewright uses `ENetConnection` directly, so not affected.
