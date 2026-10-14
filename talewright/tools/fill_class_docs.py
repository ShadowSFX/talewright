#!/usr/bin/env python3
"""Fill empty descriptions in class reference XML files (doc_classes/*.xml) from a JSON file.

Usage: python talewright/tools/fill_class_docs.py <docs.json> [--doc-dir modules/jrpg/doc_classes]
JSON shape: {"ClassName": {"brief": "...", "description": "...", "methods": {"name": "..."},
             "members": {"name": "..."}, "constants": {"NAME": "..."}, "signals": {"name": "..."},
             "theme_items": {"name": "..."}}}
Only empty slots are filled; text already present is never replaced. Text is XML-escaped and indented the way
`--doctool` writes it, so running `--doctool` afterwards produces no diff. Unknown names are reported (exit 1).
After filling, the script lists every slot that is still empty in the touched files.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


def escape(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def block(text: str, indent: str) -> str:
    return "".join(f"{indent}{line}\n" for line in escape(text).split("\n"))


def fill(xml: str, kind: str, name: str, text: str) -> tuple[str, bool]:
    t = "\t"
    if kind == "brief":
        rx = re.compile(r"(\t<brief_description>\n)(\t</brief_description>)")
        return rx.subn(lambda m: m.group(1) + block(text, t * 2) + m.group(2), xml, count=1)[0], True
    if kind == "description":
        rx = re.compile(r"(\n\t<description>\n)(\t</description>)")
        return rx.subn(lambda m: m.group(1) + block(text, t * 2) + m.group(2), xml, count=1)[0], True
    if kind in ("methods", "signals"):
        tag = "method" if kind == "methods" else "signal"
        rx = re.compile(
            r"(\t\t<" + tag + r' name="' + re.escape(name) + r'"[^>]*>\n(?:\t\t\t(?!<description>).*\n)*'
            r"\t\t\t<description>\n)(\t\t\t</description>)"
        )
        new, n = rx.subn(lambda m: m.group(1) + block(text, t * 4) + m.group(2), xml, count=1)
        return new, n == 1 or (f'<{tag} name="{name}"' in xml)
    tag = {"members": "member", "constants": "constant", "theme_items": "theme_item"}[kind]
    rx = re.compile(r"(\t\t<" + tag + r' name="' + re.escape(name) + r'"[^>]*>\n)(\t\t</' + tag + ">)")
    new, n = rx.subn(lambda m: m.group(1) + block(text, t * 3) + m.group(2), xml, count=1)
    return new, n == 1 or (f'<{tag} name="{name}"' in xml)


def empty_slots(xml: str) -> list[str]:
    out = []
    if re.search(r"<brief_description>\n\t</brief_description>", xml):
        out.append("brief")
    out += [
        "method " + m
        for m in re.findall(
            r'<method name="(\w+)"[^>]*>\n(?:\t\t\t(?!<description>).*\n)*\t\t\t<description>\n\t\t\t</description>',
            xml,
        )
    ]
    out += ["member " + m for m in re.findall(r'<member name="([\w/]+)"[^>]*>\n\t\t</member>', xml)]
    out += ["constant " + m for m in re.findall(r'<constant name="(\w+)"[^>]*>\n\t\t</constant>', xml)]
    out += [
        "signal " + m
        for m in re.findall(
            r'<signal name="(\w+)"[^>]*>\n(?:\t\t\t(?!<description>).*\n)*\t\t\t<description>\n\t\t\t</description>',
            xml,
        )
    ]
    out += ["theme item " + m for m in re.findall(r'<theme_item name="(\w+)"[^>]*>\n\t\t</theme_item>', xml)]
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("docs")
    ap.add_argument("--doc-dir", default="modules/jrpg/doc_classes")
    args = ap.parse_args()
    data = json.loads(Path(args.docs).read_text(encoding="utf-8"))
    unknown = []
    for cls, entries in data.items():
        path = Path(args.doc_dir) / f"{cls}.xml"
        if not path.is_file():
            unknown.append(f"{cls}: no {path}")
            continue
        xml = path.read_text(encoding="utf-8")
        for kind in ("brief", "description"):
            if kind in entries:
                xml, _ = fill(xml, kind, "", entries[kind])
        for kind in ("methods", "members", "constants", "signals", "theme_items"):
            for name, text in entries.get(kind, {}).items():
                xml, found = fill(xml, kind, name, text)
                if not found:
                    unknown.append(f"{cls}: no {kind[:-1]} {name}")
        path.write_bytes(xml.encode("utf-8"))
        for slot in empty_slots(xml):
            print(f"still empty: {cls} {slot}")
    for u in unknown:
        print("unknown:", u)
    return 1 if unknown else 0


if __name__ == "__main__":
    sys.exit(main())
