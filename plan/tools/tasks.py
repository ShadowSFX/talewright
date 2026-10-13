#!/usr/bin/env python3
"""Talewright task tracker CLI.

plan/tasks.json is the source of truth. ROADMAP.md is rendered from it and
PROGRESS.md gets its header and log updated by the commands below.

Usage (run from the repository root):
    python plan/tools/tasks.py validate
    python plan/tools/tasks.py render
    python plan/tools/tasks.py next [-n 5]
    python plan/tools/tasks.py show <id>
    python plan/tools/tasks.py start <id>
    python plan/tools/tasks.py done <id> -m "what was done"
    python plan/tools/tasks.py block <id> -m "exact reason"
    python plan/tools/tasks.py defer <id> -m "reason"
    python plan/tools/tasks.py reopen <id> -m "reason"
    python plan/tools/tasks.py stamp            # fill commit hashes of tasks marked "pending"
    python plan/tools/tasks.py import <file.json>  # add/replace tasks from a JSON list
    python plan/tools/tasks.py log -m "free text" [--id <id>]
    python plan/tools/tasks.py stats

Commit hashes: a commit cannot contain its own hash, so `done` stores
"pending"; after `git commit`, `stamp` fills the hash from `git log --grep`
and that stamp is included in the next task's commit.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PLAN = ROOT / "plan"
TASKS = PLAN / "tasks.json"
ROADMAP = PLAN / "ROADMAP.md"
PROGRESS = PLAN / "PROGRESS.md"

STATUSES = ("pending", "in_progress", "done", "blocked", "deferred")
CLOSED = ("done", "deferred")
REQUIRED = (
    "id",
    "title",
    "description",
    "parent",
    "dependencies",
    "status",
    "priority",
    "acceptance_criteria",
    "test_strategy",
    "files",
    "commit",
    "notes",
)
BOX = {"pending": "[ ]", "in_progress": "[~]", "done": "[x]", "blocked": "[!]", "deferred": "[-]"}


def now() -> str:
    return _dt.datetime.now().astimezone().strftime("%Y-%m-%d %H:%M %z")


def id_key(task_id: str) -> tuple:
    return tuple(int(p) if p.isdigit() else p for p in task_id.split("."))


def load() -> dict:
    with TASKS.open(encoding="utf-8") as f:
        return json.load(f)


def save(data: dict) -> None:
    data["tasks"].sort(key=lambda t: id_key(t["id"]))
    text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    TASKS.write_text(text, encoding="utf-8", newline="\n")


def index(data: dict) -> dict:
    return {t["id"]: t for t in data["tasks"]}


def children(data: dict, task_id: str | None) -> list:
    return sorted((t for t in data["tasks"] if t["parent"] == task_id), key=lambda t: id_key(t["id"]))


def get(data: dict, task_id: str) -> dict:
    t = index(data).get(task_id)
    if t is None:
        sys.exit(f"error: unknown task id {task_id!r}")
    return t


def validate(data: dict) -> list:
    errors = []
    ids = set()
    idx = {}
    for t in data["tasks"]:
        for key in REQUIRED:
            if key not in t:
                errors.append(f"{t.get('id', '?')}: missing field {key!r}")
        tid = t.get("id")
        if tid in ids:
            errors.append(f"{tid}: duplicate id")
        ids.add(tid)
        idx[tid] = t
        if t.get("status") not in STATUSES:
            errors.append(f"{tid}: invalid status {t.get('status')!r}")
        if not isinstance(t.get("priority"), int) or not 1 <= t["priority"] <= 4:
            errors.append(f"{tid}: priority must be an int 1 (highest) .. 4")
        if not t.get("acceptance_criteria"):
            errors.append(f"{tid}: acceptance_criteria is empty")
        if not t.get("test_strategy"):
            errors.append(f"{tid}: test_strategy is empty")
        if t.get("status") in ("blocked", "deferred") and not t.get("notes"):
            errors.append(f"{tid}: {t['status']} task needs a reason in notes")
    for t in data["tasks"]:
        tid = t["id"]
        parent = t.get("parent")
        if parent is not None:
            if parent not in idx:
                errors.append(f"{tid}: parent {parent} does not exist")
            elif not tid.startswith(parent + "."):
                errors.append(f"{tid}: id is not nested under parent {parent}")
        elif "." in tid:
            errors.append(f"{tid}: nested id without parent")
        for dep in t.get("dependencies", []):
            if dep not in idx:
                errors.append(f"{tid}: dependency {dep} does not exist")
            if dep == tid:
                errors.append(f"{tid}: depends on itself")
    # Dependency cycles (deps + implicit parent links).
    state: dict = {}

    def visit(tid: str, stack: list) -> None:
        if state.get(tid) == 2:
            return
        if state.get(tid) == 1:
            errors.append("dependency cycle: " + " -> ".join(stack + [tid]))
            return
        state[tid] = 1
        for dep in idx[tid].get("dependencies", []):
            if dep in idx:
                visit(dep, stack + [tid])
        state[tid] = 2

    for tid in idx:
        visit(tid, [])
    return errors


def deps_done(data: dict, t: dict) -> bool:
    idx = index(data)
    for dep in t["dependencies"]:
        d = idx[dep]
        if d["status"] not in CLOSED:
            # A dependency on a container is satisfied when all its descendants are closed.
            if not children(data, dep) or not all_closed(data, dep):
                return False
    return True


def all_closed(data: dict, task_id: str) -> bool:
    kids = children(data, task_id)
    if not kids:
        return get(data, task_id)["status"] in CLOSED
    return all(all_closed(data, k["id"]) for k in kids)


def ancestors_ok(data: dict, t: dict) -> bool:
    idx = index(data)
    p = t["parent"]
    while p is not None:
        pt = idx[p]
        if pt["status"] in ("blocked", "deferred"):
            return False
        if not deps_done(data, pt):
            return False
        p = pt["parent"]
    return True


def ready(data: dict) -> list:
    out = []
    for t in data["tasks"]:
        if t["status"] != "pending" or children(data, t["id"]):
            continue
        if deps_done(data, t) and ancestors_ok(data, t):
            out.append(t)
    out.sort(key=lambda t: (t["priority"], id_key(t["id"])))
    return out


def close_containers(data: dict) -> list:
    """Mark containers done when every descendant is closed. Returns closed ids."""
    closed = []
    changed = True
    while changed:
        changed = False
        for t in data["tasks"]:
            kids = children(data, t["id"])
            if kids and t["status"] not in CLOSED and all(k["status"] in CLOSED for k in kids):
                t["status"] = "done"
                t["commit"] = t["commit"] or "children"
                closed.append(t["id"])
                changed = True
    return closed


def render(data: dict) -> None:
    lines = [
        "# Talewright Roadmap",
        "",
        "_Generated from `plan/tasks.json` by `python plan/tools/tasks.py render`. Do not edit by hand._",
        "",
        "Legend: `[x]` done, `[~]` in progress, `[ ]` pending, `[!]` blocked, `[-]` deferred. "
        "Priority P1 (highest) .. P4.",
        "",
    ]
    counts = {s: 0 for s in STATUSES}
    for t in data["tasks"]:
        counts[t["status"]] += 1
    lines.append(
        "**Totals:** "
        + ", ".join(f"{k}: {v}" for k, v in counts.items())
        + f" (of {len(data['tasks'])})"
    )
    lines.append("")

    def emit(task: dict, depth: int) -> None:
        if depth == 0:
            lines.append(f"## {BOX[task['status']]} {task['id']} {task['title']}")
            lines.append("")
            lines.append(task["description"])
            lines.append("")
        else:
            indent = "  " * (depth - 1)
            extra = []
            if task["dependencies"]:
                extra.append("deps: " + ", ".join(task["dependencies"]))
            if task["commit"] and task["commit"] not in ("children",):
                extra.append("commit: " + task["commit"])
            if task["status"] in ("blocked", "deferred") and task["notes"]:
                extra.append(task["status"] + ": " + task["notes"].splitlines()[0])
            suffix = f" _({'; '.join(extra)})_" if extra else ""
            lines.append(f"{indent}- {BOX[task['status']]} **{task['id']}** {task['title']} (P{task['priority']}){suffix}")
        for k in children(data, task["id"]):
            emit(k, depth + 1)
        if depth == 0:
            lines.append("")

    for top in children(data, None):
        emit(top, 0)
    ROADMAP.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8", newline="\n")


HEADER_RE = re.compile(r"<!-- status:begin -->.*?<!-- status:end -->", re.S)


def update_progress(data: dict, log_line: str | None) -> None:
    text = PROGRESS.read_text(encoding="utf-8") if PROGRESS.exists() else ""
    current = [t for t in data["tasks"] if t["status"] == "in_progress"]
    done = [t for t in data["tasks"] if t["status"] == "done" and t["commit"] != "children"]
    last = data.get("last_completed")
    nxt = ready(data)
    block = [
        "<!-- status:begin -->",
        f"- **Updated:** {now()}",
        "- **Current task:** "
        + (", ".join(f"{t['id']} {t['title']}" for t in current) if current else "none"),
        "- **Last completed:** " + (last if last else "none"),
        "- **Next task:** " + (f"{nxt[0]['id']} {nxt[0]['title']}" if nxt else "none ready"),
        f"- **Done leaf tasks:** {len(done)}",
        "<!-- status:end -->",
    ]
    if HEADER_RE.search(text):
        text = HEADER_RE.sub("\n".join(block), text)
    else:
        text = "# Talewright Progress\n\n" + "\n".join(block) + "\n\n## Log\n\n| Time | Task | What | Commit |\n|---|---|---|---|\n" + text
    if log_line:
        text = text.rstrip("\n") + "\n" + log_line + "\n"
    PROGRESS.write_text(text, encoding="utf-8", newline="\n")


def md_cell(s: str) -> str:
    return s.replace("|", "\\|").replace("\n", " ")


def cmd_set_status(args, status: str) -> None:
    data = load()
    t = get(data, args.id)
    msg = args.message or ""
    if status in ("blocked", "deferred") and not msg:
        sys.exit(f"error: {status} needs -m with the exact reason")
    if status == "in_progress" and children(data, t["id"]):
        sys.exit("error: start a leaf task, not a container")
    if status == "in_progress" and not deps_done(data, t):
        sys.exit("error: dependencies are not done: " + ", ".join(t["dependencies"]))
    t["status"] = status
    if status == "done":
        t["commit"] = "pending"
        data["last_completed"] = f"{t['id']} {t['title']}"
    if msg and status in ("blocked", "deferred", "pending"):
        t["notes"] = (msg + ("\n" + t["notes"] if t["notes"] else "")).strip()
    closed = close_containers(data) if status in CLOSED else []
    save(data)
    render(data)
    verb = {"in_progress": "started", "done": "done", "blocked": "BLOCKED", "deferred": "deferred", "pending": "reopened"}[status]
    what = f"{verb}: {t['title']}" + (f" — {msg}" if msg else "")
    if closed:
        what += f" (closed containers: {', '.join(closed)})"
    commit = "pending" if status == "done" else ""
    update_progress(data, f"| {now()} | {t['id']} | {md_cell(what)} | {commit} |")
    print(f"{t['id']} -> {status}")
    if closed:
        print("closed containers:", ", ".join(closed))


def cmd_stamp(_args) -> None:
    data = load()
    changed = []
    for t in data["tasks"]:
        if t["commit"] != "pending":
            continue
        out = subprocess.run(
            ["git", "log", "-n", "1", "--format=%h", "--grep", f"^{re.escape(t['id'])}: "],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        ).stdout.strip()
        if out:
            t["commit"] = out
            changed.append((t["id"], out))
    if changed:
        save(data)
        render(data)
        text = PROGRESS.read_text(encoding="utf-8")
        for tid, h in changed:
            text = re.sub(rf"(\| {re.escape(tid)} \| [^\n]*\| )pending \|", rf"\g<1>{h} |", text)
        PROGRESS.write_text(text, encoding="utf-8", newline="\n")
    for tid, h in changed:
        print(f"{tid}: {h}")
    if not changed:
        print("nothing to stamp")


def cmd_import(args) -> None:
    data = load()
    idx = index(data)
    new = json.loads(Path(args.file).read_text(encoding="utf-8"))
    for t in new:
        for key, default in (
            ("dependencies", []),
            ("status", "pending"),
            ("priority", 2),
            ("files", []),
            ("commit", ""),
            ("notes", ""),
        ):
            t.setdefault(key, default)
        if t["id"] in idx:
            idx[t["id"]].update(t)
        else:
            data["tasks"].append(t)
            idx[t["id"]] = t
    errors = validate(data)
    if errors:
        sys.exit("validation failed:\n  " + "\n  ".join(errors))
    save(data)
    render(data)
    print(f"imported {len(new)} tasks")


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("validate")
    sub.add_parser("render")
    sub.add_parser("stats")
    sub.add_parser("stamp")
    n = sub.add_parser("next")
    n.add_argument("-n", type=int, default=5)
    s = sub.add_parser("show")
    s.add_argument("id")
    for name in ("start", "done", "block", "defer", "reopen"):
        c = sub.add_parser(name)
        c.add_argument("id")
        c.add_argument("-m", "--message", default="")
    i = sub.add_parser("import")
    i.add_argument("file")
    lg = sub.add_parser("log")
    lg.add_argument("-m", "--message", required=True)
    lg.add_argument("--id", default="-")
    args = p.parse_args()

    if args.cmd == "validate":
        errors = validate(load())
        if errors:
            print("\n".join(errors))
            sys.exit(1)
        print("ok")
    elif args.cmd == "render":
        data = load()
        render(data)
        update_progress(data, None)
        print("rendered")
    elif args.cmd == "stats":
        data = load()
        counts = {s: 0 for s in STATUSES}
        for t in data["tasks"]:
            counts[t["status"]] += 1
        print(json.dumps(counts))
    elif args.cmd == "next":
        for t in ready(load())[: args.n]:
            print(f"{t['id']}\tP{t['priority']}\t{t['title']}")
    elif args.cmd == "show":
        print(json.dumps(get(load(), args.id), indent=2, ensure_ascii=False))
    elif args.cmd == "start":
        cmd_set_status(args, "in_progress")
    elif args.cmd == "done":
        cmd_set_status(args, "done")
    elif args.cmd == "block":
        cmd_set_status(args, "blocked")
    elif args.cmd == "defer":
        cmd_set_status(args, "deferred")
    elif args.cmd == "reopen":
        cmd_set_status(args, "pending")
    elif args.cmd == "stamp":
        cmd_stamp(args)
    elif args.cmd == "import":
        cmd_import(args)
    elif args.cmd == "log":
        data = load()
        update_progress(data, f"| {now()} | {args.id} | {md_cell(args.message)} | |")
        print("logged")


if __name__ == "__main__":
    main()
