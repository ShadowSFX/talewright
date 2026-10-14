#!/usr/bin/env python3
"""Run the CI static checks locally before pushing (the GitHub "Static checks" job, .github/workflows/static_checks.yml).

CI runs `prek run --files <changed files>` with the repository's .pre-commit-config.yaml. This script runs the same
hooks on the files a push would bring (default: everything that differs from origin/talewright, committed or not,
plus untracked files), then the fork's own checks: Talewright license headers (Godot's copyright-headers hook skips
the fork modules), the jrpg core include boundary, the upstream-patch ledger, clang's diagnostics on the fork's
C++ (talewright/tools/clang_check.py; skipped when clang-tidy or compile_commands.json is missing), and clangd-tidy on
the changed C/C++ files, as CI's "Style checks via clangd-tidy" step runs it (every file analyzed on its own, so
headers must include what they use; any diagnostic fails, CI's default --fail-on-severity hint). Install it with
`python -m pip install --user clangd clangd-tidy`.

Usage (any directory inside the repository):
    python talewright/tools/static_checks.py            # files changed since origin/talewright
    python talewright/tools/static_checks.py --all      # every file the fork adds or changes vs 4.7.2-stable
    python talewright/tools/static_checks.py --base REF # files changed since REF
    python talewright/tools/static_checks.py --allow-skips  # accept skipped clang checks (tools missing)
Requires prek (`python -m pip install --user prek`). Hooks such as codespell and ruff rewrite files: the script lists
the files a hook changed so they can be reviewed (codespell can "correct" identifiers and labels) before committing.
Exit code 0 only when every check passes without modifying anything. A check that cannot run (clang-tidy, the
compile database or clangd-tidy missing) counts as a failure unless --allow-skips is given: a skipped check would
let a push through that CI then rejects.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path

FORK_MODULES = ("modules/claude_bridge/", "modules/jrpg/", "modules/mmo/")
CPP_SUFFIXES = (".h", ".hpp", ".cpp", ".inc")


# Fork C++ must not use these without their standard header; the engine has portable equivalents.
PORTABILITY = {"std::size(": "std_size( (core/typedefs.h)"}


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


def find_tool(name: str) -> str | None:
    found = shutil.which(name)
    if found:
        return found
    user_scripts = Path(sysconfig.get_path("scripts", f"{os.name}_user"))
    for candidate in (user_scripts / (name + ".exe"), user_scripts / name):
        if candidate.is_file():
            return str(candidate)
    return None


def run_clangd_tidy(root: Path, files: list[str]) -> tuple[int, str]:
    """Returns (exit code, last line); exit code 2 when clangd-tidy or the compile database is missing."""
    sources = [f for f in files if f.endswith(CPP_SUFFIXES) and not f.startswith("thirdparty/")]
    if not sources:
        return 0, "no C/C++ files changed"
    tidy, clangd = find_tool("clangd-tidy"), find_tool("clangd")
    if not tidy or not clangd or not (root / "compile_commands.json").is_file():
        return 2, "clangd-tidy, clangd or compile_commands.json not found"
    env = dict(os.environ, PYTHONUTF8="1")  # clangd-tidy reads sources with the default encoding.
    out = subprocess.run(
        [tidy, "--clangd-executable", clangd, "-p", str(root), "-j", "16", "--color", "never", *sources],
        cwd=root,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=env,
    )
    diagnostics = [
        ln
        for ln in out.stdout.splitlines()
        if ": Error:" in ln or ": Warning:" in ln or ": Information:" in ln or ": Hint:" in ln
    ]
    for d in diagnostics:
        print("   ", d)
    return (1 if diagnostics else 0), f"{len(diagnostics)} diagnostics in {len(sources)} files"


def content_hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else ""


def shebang_modes(root: Path, files: list[str]) -> list[str]:
    """Files that start with a shebang but are not mode 100755 in the git index."""
    listed = subprocess.run(["git", "ls-files", "-s", "--", *files], cwd=root, capture_output=True, text=True).stdout
    modes = {line.split("	", 1)[1]: line.split()[0] for line in listed.splitlines() if "	" in line}
    problems = []
    for f in files:
        path = root / f
        if not path.is_file():
            continue
        with path.open("rb") as handle:
            if handle.read(2) != b"#!":
                continue
        if modes.get(f) != "100755":
            mode = modes.get(f, "not added yet")
            problems.append(f"{f}: has a shebang but git mode is {mode}; run git add --chmod=+x {f}")
    return problems


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="check every file that differs from 4.7.2-stable")
    ap.add_argument("--base", default="origin/talewright")
    ap.add_argument("--allow-skips", action="store_true", help="pass even when a check could not run")
    args = ap.parse_args()
    root = repo_root()
    files = changed_files(root, "4.7.2-stable" if args.all else args.base)
    if not files:
        print("no changed files")
        return 0
    print(f"checking {len(files)} files")
    ok = True
    skipped = []

    before = {f: content_hash(root / f) for f in files}
    prek = subprocess.run([sys.executable, "-m", "prek", "run", "--color", "never", "--files", *files], cwd=root)
    if prek.returncode != 0:
        ok = False
        print("[FAIL] prek hooks (see output above)")
    else:
        print("[PASS] prek hooks")
    rewritten = [f for f in files if content_hash(root / f) != before[f]]
    if rewritten:
        ok = False
        print(
            "[REVIEW] hooks rewrote these files; check the changes (identifiers, labels, URLs, quotes) before committing:"
        )
        for f in rewritten:
            print("   ", f)

    # CI's "check that scripts with shebangs are executable" reads the git file mode, which Windows checkouts do not
    # show: a script added on Windows is not executable unless added with --chmod=+x (CI run 38525200924).
    modes = shebang_modes(root, files)
    print(("[PASS]" if not modes else "[FAIL]") + " executable scripts (git file mode)")
    for problem in modes:
        print("   ", problem)
    ok &= not modes

    cpp = [f for f in files if f.startswith(FORK_MODULES) and f.endswith(CPP_SUFFIXES)]
    if cpp:
        r = subprocess.run([sys.executable, "talewright/tools/add_header.py", "--check", *cpp], cwd=root)
        print(("[PASS]" if r.returncode == 0 else "[FAIL]") + f" Talewright headers ({len(cpp)} files)")
        ok &= r.returncode == 0

    # Standard-library names whose header MSVC's STL pulls in transitively but libc++ (macOS, CI run 38510290436)
    # does not: use the engine's helpers instead.
    portability = []
    for f in cpp:
        for number, line in enumerate((root / f).read_text(encoding="utf-8").splitlines(), 1):
            for name, use in PORTABILITY.items():
                if name in line:
                    portability.append(f"{f}:{number}: {name} -> {use}")
    print(("[PASS]" if not portability else "[FAIL]") + f" portable standard-library use ({len(cpp)} files)")
    for finding in portability:
        print("   ", finding)
    ok &= not portability

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

    # The generated default assets match their generator and manifest (pixels, audio, importer settings).
    assets = subprocess.run(
        [sys.executable, "talewright/tools/generate_assets.py", "--check"], cwd=root, capture_output=True, text=True
    )
    last = (assets.stdout.strip().splitlines() or [""])[-1]
    if assets.returncode == 2:
        print("[SKIP] generated assets: " + last)
        skipped.append("generated assets")
    else:
        print(("[PASS] " if assets.returncode == 0 else "[FAIL] ") + "generated assets: " + last)
        if assets.returncode != 0:
            print(assets.stdout)
        ok &= assets.returncode == 0

    # clang's diagnostics on every fork translation unit (CI builds with clang on most platforms; MSVC misses some).
    clang = subprocess.run(
        [sys.executable, "talewright/tools/clang_check.py"], cwd=root, capture_output=True, text=True
    )
    last = (clang.stdout.strip().splitlines() or [""])[-1]
    if clang.returncode == 2:
        print("[SKIP] clang diagnostics: " + last)
        skipped.append("clang diagnostics")
    else:
        print(("[PASS] " if clang.returncode == 0 else "[FAIL] ") + "clang diagnostics: " + last)
        if clang.returncode != 0:
            print(clang.stdout)
        ok &= clang.returncode == 0

    # CI analyzes each changed C/C++ file on its own with clangd (headers included).
    code, last = run_clangd_tidy(root, files)
    if code == 2:
        print("[SKIP] clangd-tidy: " + last)
        skipped.append("clangd-tidy")
    else:
        print(("[PASS] " if code == 0 else "[FAIL] ") + "clangd-tidy: " + last)
        ok &= code == 0

    if skipped and not args.allow_skips:
        ok = False
        print("[FAIL] checks that could not run: " + ", ".join(skipped) + " (install them, or pass --allow-skips)")
    if ok and skipped:
        print("PASS, skipped: " + ", ".join(skipped))
    else:
        print("ALL PASS" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
