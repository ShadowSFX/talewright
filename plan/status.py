#!/usr/bin/env python3
"""Talewright completion status: task counts per status, blocked/deferred reasons, Phase 5 sweep table state.

Usage (repository root): python plan/status.py
The goal is reached when every task is done, or blocked/deferred with its reason in `notes`, and the Phase 5 sweep
table (plan/evidence/phase5_sweep.md, task 47) is committed. Exit code 0 when reached, 1 otherwise.
"""

from __future__ import annotations

import io
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TASKS = ROOT / "plan" / "tasks.json"
SWEEP = "plan/evidence/phase5_sweep.md"
STATUSES = ("done", "in_progress", "pending", "blocked", "deferred")


def sweep_state() -> str:
    path = ROOT / SWEEP
    if not path.is_file():
        return "missing"
    tracked = (
        subprocess.run(["git", "ls-files", "--error-unmatch", SWEEP], cwd=ROOT, capture_output=True).returncode == 0
    )
    if not tracked:
        return "present but not committed"
    dirty = subprocess.run(["git", "status", "--porcelain", "--", SWEEP], cwd=ROOT, capture_output=True, text=True)
    has_table = "| " in path.read_text(encoding="utf-8")
    state = "committed" if not dirty.stdout.strip() else "committed, with uncommitted edits"
    return state + ("" if has_table else " (no findings table yet)")


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    tasks = json.loads(TASKS.read_text(encoding="utf-8"))["tasks"]
    parents = {t.get("parent") for t in tasks if t.get("parent")}
    leaves = [t for t in tasks if t["id"] not in parents]
    all_counts = Counter(t["status"] for t in tasks)
    leaf_counts = Counter(t["status"] for t in leaves)

    print(f"Tasks: {len(tasks)} total ({len(leaves)} leaves, {len(tasks) - len(leaves)} containers)")
    print(f"{'status':12} {'all':>5} {'leaves':>7}")
    for s in STATUSES:
        print(f"{s:12} {all_counts.get(s, 0):>5} {leaf_counts.get(s, 0):>7}")

    closed = [t for t in tasks if t["status"] in ("blocked", "deferred")]
    print(f"\nBlocked/deferred ({len(closed)}):")
    missing_reason = []
    for t in sorted(closed, key=lambda t: [int(x) for x in t["id"].split(".")]):
        reason = (t.get("notes") or "").strip().splitlines()
        if not reason:
            missing_reason.append(t["id"])
        print(f"  {t['id']:8} {t['status']:9} {reason[0] if reason else '(NO REASON RECORDED)'}")
    if not closed:
        print("  (none)")

    sweep = sweep_state()
    print(f"\nPhase 5 sweep table ({SWEEP}): {sweep}")

    open_tasks = all_counts.get("pending", 0) + all_counts.get("in_progress", 0)
    reached = open_tasks == 0 and not missing_reason and sweep == "committed"
    print(f"Goal reached: {'yes' if reached else 'no'} ({open_tasks} tasks still open)")
    return 0 if reached else 1


if __name__ == "__main__":
    sys.exit(main())
