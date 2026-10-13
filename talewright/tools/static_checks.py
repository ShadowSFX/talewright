#!/usr/bin/env python3
"""Run the CI static checks locally before pushing (the GitHub "Static checks" job, .github/workflows/static_checks.yml).

CI runs `prek run --files <changed files>` with the repository's .pre-commit-config.yaml. This script runs the same
hooks on the files a push would bring (default: everything that differs from origin/talewright, committed or not,
plus untracked files), then the fork's own checks: Talewright license headers (Godot's copyright-headers hook skips
the fork modules), the jrpg core include boundary and the upstream-patch ledger.

Usage (any directory inside the repository):
    python talewright/tools/static_checks.py            # files changed since origin/talewright
    python talewright/tools/static_checks.py --all      # every file the fork adds or changes vs 4.7.2-stable
    python talewright/tools/static_checks.py --base REF # files changed since REF
Requires prek (`python -m pip install --user prek`). Hooks such as codespell and ruff rewrite files: the script lists
the files a hook changed so they can be reviewed (codespell can "correct" identifiers and labels) before committing.
Exit code 0 only when every check passes without modifying anything.
"""

from __future__ import annotations

import argparse
import io
import subprocess
import sys
from pathlib import Path

FORK_MODULES = ("modules/claude_bridge/", "modules/jrpg/", "modules/mmo/")
CPP_SUFFIXES = (".h", ".hpp", ".cpp", ".inc")


def git(root: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=root, capture_output=True, text=True, check=True).stdout


def repo_root() -> Path:
    # `git rev-parse` returns the path with its real letter case. Running mypy from a differently cased path makes
    # it see files as outside the project and report duplicate modules.
    here = Path(__file__).resolve().parent
    return Path(git(here, "rev-parse", "--show-toplevel").strip())


def changed_files(root: Path, base: str) -> list[str]:
    files = set(git(root, "diff", "--name-only", "--no-renames", "-z", base, "--").split("\0"))
    files |= set(git(root, "ls-files", "--others", "--exclude-standard", "-z").split("\0"))
    return sorted(f for f in files if f and (root / f).is_file())


def status_snapshot(root: Path) -> str:
    return git(root, "status", "--porcelain", "-z")


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="check every file that differs from 4.7.2-stable")
    ap.add_argument("--base", default="origin/talewright")
    args = ap.parse_args()
    root = repo_root()
    files = changed_files(root, "4.7.2-stable" if args.all else args.base)
    if not files:
        print("no changed files")
        return 0
    print(f"checking {len(files)} files")
    ok = True

    before = status_snapshot(root)
    diff_before = git(root, "diff")
    prek = subprocess.run([sys.executable, "-m", "prek", "run", "--color", "never", "--files", *files], cwd=root)
    if prek.returncode != 0:
        ok = False
        print("[FAIL] prek hooks (see output above)")
    else:
        print("[PASS] prek hooks")
    if status_snapshot(root) != before or git(root, "diff") != diff_before:
        ok = False
        modified = [ln[3:] for ln in git(root, "status", "--porcelain").splitlines()]
        print("[REVIEW] hooks rewrote files; inspect `git diff` before committing (identifiers, labels, URLs):")
        for m in modified:
            print("   ", m)

    cpp = [f for f in files if f.startswith(FORK_MODULES) and f.endswith(CPP_SUFFIXES)]
    if cpp:
        r = subprocess.run([sys.executable, "talewright/tools/add_header.py", "--check", *cpp], cwd=root)
        print(("[PASS]" if r.returncode == 0 else "[FAIL]") + f" Talewright headers ({len(cpp)} files)")
        ok &= r.returncode == 0

    for name, cmd in (
        ("jrpg core include boundary", [sys.executable, "modules/jrpg/tests/check_core_includes.py"]),
        ("upstream patch ledger", [sys.executable, "plan/tools/core_diff_check.py"]),
    ):
        check = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
        print(
            ("[PASS] " if check.returncode == 0 else "[FAIL] ")
            + name
            + ": "
            + (check.stdout.strip().splitlines() or [""])[-1]
        )
        if check.returncode != 0:
            print(check.stdout)
        ok &= check.returncode == 0

    print("ALL PASS" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
