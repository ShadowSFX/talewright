#!/usr/bin/env python3
"""Regression smoke test for the plain (non-JRPG) samples. Run after every epic.

Checks, per sample in talewright/samples/{plain_2d,plain_3d}:
  1. editor: `--headless --editor --path <sample> --quit` imports and exits without errors;
  2. run:    `--headless --path <sample> -- --smoke` prints <NAME>_OK and exits 0 without errors;
and for plain_2d additionally:
  3. export: exports "Windows Desktop" with the custom template bin/godot.windows.template_release.x86_64.exe
             (export_presets.cfg is generated, git-ignored) and runs the exported exe with `-- --smoke`.

Usage: python talewright/tools/smoke_samples.py [--editor-bin PATH] [--no-export]
Exit code 1 on any failure.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SAMPLES = ROOT / "talewright" / "samples"
EXPORTS = ROOT / "talewright" / "build_logs" / "exports"
EDITOR_BIN = ROOT / "bin" / "godot.windows.editor.x86_64.console.exe"
TEMPLATE = ROOT / "bin" / "godot.windows.template_release.x86_64.exe"
ERROR_RE = re.compile(r"^(ERROR|SCRIPT ERROR|USER ERROR)\b|\berror\b.*\bat:", re.M)

PRESET = """[preset.0]

name="Windows Desktop"
platform="Windows Desktop"
runnable=true
dedicated_server=false
custom_features=""
export_filter="all_resources"
include_filter=""
exclude_filter=""
export_path="{export_path}"

[preset.0.options]

custom_template/debug=""
custom_template/release="{template}"
binary_format/embed_pck=true
application/modify_resources=false
"""


def run(cmd: list, timeout: int = 180) -> tuple:
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout)
    return proc.returncode, proc.stdout + proc.stderr


def check(label: str, code: int, out: str, marker: str | None = None) -> bool:
    problems = []
    if code != 0:
        problems.append(f"exit code {code}")
    errors = ERROR_RE.findall(out)
    if errors:
        problems.append(f"{len(errors)} error line(s)")
    if marker and marker not in out:
        problems.append(f"marker {marker} missing")
    status = "PASS" if not problems else "FAIL (" + ", ".join(problems) + ")"
    print(f"[{status}] {label}")
    if problems:
        print("  " + "\n  ".join(out.strip().splitlines()[-25:]))
    return not problems


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--editor-bin", default=str(EDITOR_BIN))
    ap.add_argument("--no-export", action="store_true")
    args = ap.parse_args()
    exe = args.editor_bin
    ok = True
    for name, marker in (("plain_2d", "PLAIN2D_OK"), ("plain_3d", "PLAIN3D_OK")):
        path = SAMPLES / name
        code, out = run([exe, "--headless", "--editor", "--path", str(path), "--quit"])
        ok &= check(f"{name}: editor opens and imports", code, out)
        code, out = run([exe, "--headless", "--path", str(path), "--", "--smoke"])
        ok &= check(f"{name}: runs", code, out, marker)

    if not args.no_export:
        if not TEMPLATE.exists():
            print(f"[FAIL] plain_2d export: template missing ({TEMPLATE.relative_to(ROOT)})")
            return 1
        out_dir = EXPORTS / "plain_2d"
        out_dir.mkdir(parents=True, exist_ok=True)
        exported = out_dir / "plain_2d.exe"
        if exported.exists():
            exported.unlink()
        project = SAMPLES / "plain_2d"
        (project / "export_presets.cfg").write_text(
            PRESET.format(export_path=exported.as_posix(), template=TEMPLATE.as_posix()), encoding="utf-8", newline="\n"
        )
        code, out = run([exe, "--headless", "--path", str(project), "--export-release", "Windows Desktop", str(exported)], 300)
        ok &= check("plain_2d: export with custom template", code, out) and exported.exists()
        if exported.exists():
            code, out = run([str(exported), "--headless", "--", "--smoke"])
            ok &= check("plain_2d: exported exe runs", code, out, "PLAIN2D_OK")
    print("ALL PASS" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
