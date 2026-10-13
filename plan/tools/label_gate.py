#!/usr/bin/env python3
"""Field-level coverage gate for the RPG Maker inventory (task 2.5, review finding 15).

toc_gate.py proves every official help page is cited; this checks that the field/section labels of each page
(the text of its <h3>/<h4> headings and <dt> terms) appear somewhere in the inventory. Pages are read from the
cache filled by verbatim_check.py (talewright/build_logs/mz_help_cache). Matching is case-insensitive on
normalized words; labels listed in plan/research/label_gate_waivers.txt (one "page|label|reason" per line)
are accepted with their reason.

Usage: python plan/tools/label_gate.py [--verbose]
Exit code 1 if any unwaived label is missing.
"""

import argparse
import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "talewright" / "build_logs" / "mz_help_cache"
LABEL_RE = re.compile(r"(?is)<(h3|h4|dt)[^>]*>(.*?)</\1>")


def norm(text: str) -> str:
    text = html.unescape(re.sub(r"(?s)<[^>]+>", " ", text)).lower()
    text = re.sub(r"\[[^\]]*\]", " ", text)  # "[SV] Motion" -> "motion" (bracket tags vary in the slices)
    return " ".join(re.findall(r"[a-z0-9]+", text))


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    toc = (ROOT / "plan/research/mz_help_toc.md").read_text(encoding="utf-8")
    exempt = {"02.html", "04.html", "05.html"}  # License, trademarks, credits (same exemptions as toc_gate.py).
    pages = sorted(set(re.findall(r"MZ_help-en/([\w.]+\.html)", toc)) - exempt)
    inventory = norm("\n".join(p.read_text(encoding="utf-8") for p in (ROOT / "plan/research/rpgmaker").glob("*.md")))
    waivers = {}
    wf = ROOT / "plan/research/label_gate_waivers.txt"
    if wf.exists():
        for line in wf.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.startswith("#"):
                page, label, reason = (line.split("|", 2) + ["", ""])[:3]
                waivers[(page.strip(), label.strip().lower())] = reason.strip()
    total = missing = waived = 0
    for page in pages:
        f = CACHE / page
        if not f.exists():
            print(f"warning: {page} not cached; run verbatim_check.py first")
            continue
        for _tag, raw in LABEL_RE.findall(f.read_text(encoding="utf-8", errors="replace")):
            label = norm(raw)
            if not label or len(label) < 3:
                continue
            total += 1
            if label in inventory:
                if args.verbose:
                    print(f"ok      {page}: {label}")
                continue
            key = (page, label)
            if key in waivers:
                waived += 1
                if args.verbose:
                    print(f"waived  {page}: {label} ({waivers[key]})")
                continue
            missing += 1
            print(f"MISSING {page}: {label}")
    print(f"labels: {total}; missing: {missing}; waived: {waived}")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
