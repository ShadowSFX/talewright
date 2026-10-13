#!/usr/bin/env python3
"""Flag possible verbatim copying from the official RPG Maker MZ help into the research inventory.

Legal rule (plan/DECISIONS.md D-020): the inventory must paraphrase; only names of fields/commands may match.
This downloads every help page listed in plan/research/mz_help_toc.md (read-only GETs, cached under
talewright/build_logs/mz_help_cache/), extracts visible text, and reports every run of N consecutive words
(default 12) shared between an inventory file and any official page.

Usage: python plan/tools/verbatim_check.py [--n 12] [files...]   (default: plan/research/rpgmaker/*.md)
Exit code 1 if any shared run is found.
"""

import argparse
import html
import re
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "talewright" / "build_logs" / "mz_help_cache"
URL_RE = re.compile(r"https://rpgmakerofficial\.com/product/MZ_help-en/[\w.]+\.html")
WORD_RE = re.compile(r"[a-z0-9']+")


def page_text(url: str) -> str:
    CACHE.mkdir(parents=True, exist_ok=True)
    cached = CACHE / url.rsplit("/", 1)[1]
    if not cached.exists():
        with urllib.request.urlopen(url, timeout=30) as r:
            cached.write_bytes(r.read())
    raw = cached.read_text(encoding="utf-8", errors="replace")
    raw = re.sub(r"(?is)<(script|style)[^>]*>.*?</\1>", " ", raw)
    return html.unescape(re.sub(r"(?s)<[^>]+>", " ", raw))


def ngrams(text: str, n: int) -> set:
    words = WORD_RE.findall(text.lower())
    return {" ".join(words[i : i + n]) for i in range(len(words) - n + 1)}


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, default=9)
    ap.add_argument("files", nargs="*")
    ap.add_argument(
        "--allow",
        default="",
        help="comma-separated item ids whose shared runs are name lists (allowed by D-020), reviewed by hand",
    )
    args = ap.parse_args()
    files = [Path(f) for f in args.files] or sorted((ROOT / "plan/research/rpgmaker").glob("*.md"))
    toc = (ROOT / "plan/research/mz_help_toc.md").read_text(encoding="utf-8")
    official = set()
    for url in sorted(set(URL_RE.findall(toc))):
        try:
            official |= ngrams(page_text(url), args.n)
        except OSError as e:
            print(f"warning: could not fetch {url}: {e}")
    allow_text = args.allow
    allow_file = ROOT / "plan/research/verbatim_allowlist.txt"
    if not allow_text and allow_file.exists():
        allow_text = allow_file.read_text(encoding="utf-8")  # Reviewed name-list items (task 2.5).
    allow = {a.strip() for a in allow_text.replace("\n", ",").split(",") if a.strip()}
    hits = 0
    for f in files:
        text = f.read_text(encoding="utf-8")
        for block in re.split(r"(?m)^### ", text)[1:]:
            title = block.splitlines()[0][:70]
            shared = ngrams(block, args.n) & official
            item_id = title.split(" ", 1)[0]
            if shared and item_id in allow:
                print(f"{f.name}: {title} -> allowed (name list): {len(shared)} shared runs")
                continue
            if shared:
                hits += 1
                print(f"{f.name}: {title}\n    e.g. \"{sorted(shared)[0]}\" ({len(shared)} shared {args.n}-word runs)")
    print(f"official {args.n}-grams: {len(official)}; blocks with shared runs: {hits}")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
