#!/usr/bin/env python3
"""Talewright build wrapper around SCons (Windows/MSVC first).

Usage (from the repository root):
    python talewright/tools/build.py editor            # editor, dev_mode (werror + tests)
    python talewright/tools/build.py editor-vanilla    # editor with every Talewright module disabled
    python talewright/tools/build.py template_release
    python talewright/tools/build.py template_debug
    python talewright/tools/build.py editor -- extra_scons_arg=value ...
Options:
    -j N          parallel jobs (default: CPU count)
    --no-kill     do not stop processes that run binaries from bin/
    --dry-run     print the SCons command line and exit

It stops processes whose executable lives in bin/ (they lock the output on Windows, LNK1104), streams the
SCons output to talewright/build_logs/<time>-<profile>.log, prints a summary (duration, exit code, errors,
warnings) and exits with SCons' exit code.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOGS = ROOT / "talewright" / "build_logs"
BIN = ROOT / "bin"
TALEWRIGHT_MODULES = ("jrpg", "mmo", "claude_bridge")

# d3d12=no: the D3D12 driver needs extra SDKs (misc/scripts/install_d3d12_sdk_windows.py); Vulkan and
# OpenGL cover Talewright's needs. Recorded in plan/research/godot_current_state.md.
COMMON = ["platform=windows", "d3d12=no"]
PROFILES = {
    "editor": [*COMMON, "target=editor", "dev_mode=yes"],
    "editor-vanilla": [*COMMON, "target=editor", "dev_mode=yes"],
    "template_release": [*COMMON, "target=template_release"],
    "template_debug": [*COMMON, "target=template_debug"],
}

ERROR_RE = re.compile(
    r"(\berror [A-Z]+\d+\b|fatal error|\berror LNK\d+|: error:|^scons: \*\*\*|Error \d+$|^ERROR\b)", re.I
)
WARNING_RE = re.compile(r"\bwarning [A-Z]+\d+\b|: warning:", re.I)


def existing_talewright_modules() -> list:
    return [m for m in TALEWRIGHT_MODULES if (ROOT / "modules" / m / "config.py").exists()]


def stop_locking_processes(target: str) -> list:
    """Stop processes running this target's binaries from bin/ (they lock the link output on Windows, LNK1104).

    Only `bin/godot.windows.<target>*` executables are stopped, so building a template does not kill a running
    editor. Returns the stopped process names.
    """
    if os.name != "nt":
        return []
    prefix = (str(BIN.resolve()) + os.sep + f"godot.windows.{target}").replace("'", "''")
    script = (
        "$p = Get-Process | Where-Object { $_.Path -and $_.Path.StartsWith('"
        + prefix
        + "', [System.StringComparison]::OrdinalIgnoreCase) }; "
        "$p | ForEach-Object { $_.ProcessName + ' ' + $_.Id }; $p | Stop-Process -Force -ErrorAction SilentlyContinue"
    )
    out = subprocess.run(
        ["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
        capture_output=True,
        text=True,
        check=False,
    ).stdout
    stopped = [ln.strip() for ln in out.splitlines() if ln.strip()]
    if stopped:
        time.sleep(1.0)  # Give Windows a moment to release file locks.
    return stopped


def main() -> int:
    # MSVC output can contain characters the Windows console code page cannot encode.
    sys.stdout.reconfigure(errors="replace")
    argv = sys.argv[1:]
    extra = []
    if "--" in argv:
        i = argv.index("--")
        argv, extra = argv[:i], argv[i + 1 :]
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("profile", choices=sorted(PROFILES))
    ap.add_argument("-j", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--no-kill", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args(argv)

    site_dir = ROOT / "talewright" / "tools" / "site_scons"
    cmd = [sys.executable, "-m", "SCons", f"--site-dir={site_dir.as_posix()}", *PROFILES[args.profile], f"-j{args.j}"]
    if args.profile == "editor-vanilla":
        cmd += [f"module_{m}_enabled=no" for m in existing_talewright_modules()]
    # misc/scripts/install_accesskit.py installs into bin/build_deps when run from Git Bash (MSYSTEM set) and
    # platform/windows/detect.py:206-209 only looks there under the same condition; pin the path so builds
    # behave the same from Git Bash and PowerShell.
    # SCons cache (SConstruct:300-301) so switching between vanilla and full builds reuses objects.
    cmd += [f"cache_path={(ROOT / 'bin' / 'scons_cache').as_posix()}", "cache_limit=40"]
    accesskit = ROOT / "bin" / "build_deps" / "accesskit"
    if accesskit.is_dir():
        cmd.append(f"accesskit_sdk_path={accesskit.as_posix()}")
    cmd += extra
    print("command:", " ".join(cmd), flush=True)
    if args.dry_run:
        return 0

    if not args.no_kill:
        target = next(a.split("=", 1)[1] for a in PROFILES[args.profile] if a.startswith("target="))
        stopped = stop_locking_processes(target)
        if stopped:
            print("stopped processes locking bin/:", ", ".join(stopped), flush=True)

    LOGS.mkdir(parents=True, exist_ok=True)
    stamp = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = LOGS / f"{stamp}-{args.profile}.log"
    errors, warnings = [], []
    env = dict(os.environ)
    env["TALEWRIGHT_VSWHERE_SHIM"] = str(site_dir / "vswhere_vc.cmd")
    start = time.monotonic()
    with log_path.open("w", encoding="utf-8", errors="replace", newline="\n") as log:
        log.write("command: " + " ".join(cmd) + "\n")
        proc = subprocess.Popen(
            cmd,
            cwd=ROOT,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            log.write(line)
            stripped = line.rstrip()
            if ERROR_RE.search(stripped):
                errors.append(stripped)
            elif WARNING_RE.search(stripped):
                warnings.append(stripped)
        code = proc.wait()
    elapsed = time.monotonic() - start

    print(f"profile: {args.profile}  exit: {code}  time: {elapsed / 60:.1f} min  log: {log_path.relative_to(ROOT)}")
    print(f"errors: {len(errors)}  warnings: {len(warnings)}")
    for line in errors[:40] if errors else warnings[:20]:
        print("  " + line[:400])
    if code != 0 and not errors:
        # Failures that are not compiler errors (e.g. a Python traceback in SConstruct): show the log tail.
        tail = log_path.read_text(encoding="utf-8", errors="replace").splitlines()[-30:]
        print("  (no compiler error matched; last 30 log lines:)")
        for line in tail:
            print("  " + line[:400])
    return code


if __name__ == "__main__":
    sys.exit(main())
