#!/usr/bin/env python3
"""Talewright task tracker CLI.

plan/tasks.json is the source of truth. ROADMAP.md is rendered from it and
PROGRESS.md gets its header and log updated by the commands below.

Usage (run from the repository root):
    python plan/tools/tasks.py validate
    python plan/tools/tasks.py render
    python plan/tools/tasks.py next [-n 5]       # ready tasks, plus in-progress ones and why nothing is ready
    python plan/tools/tasks.py doctor           # resume diagnostics: in-progress tasks, unstamped commits, dirty paths
    python plan/tools/tasks.py show <id>
    python plan/tools/tasks.py start <id>
    python plan/tools/tasks.py done <id> -m "what was done"
    python plan/tools/tasks.py block <id> -m "exact reason"
    python plan/tools/tasks.py defer <id> -m "reason"
    python plan/tools/tasks.py reopen <id> -m "reason"
    python plan/tools/tasks.py stamp            # fill commit hashes of tasks marked "pending"
    python plan/tools/tasks.py import <file.json>  # add new tasks / update fields of existing ones
    python plan/tools/tasks.py log -m "free text" [--id <id>]
    python plan/tools/tasks.py stats

Commit hashes: a commit cannot contain its own hash, so `done` stores
"pending"; after `git commit`, `stamp` fills the hash from `git log --grep`
and that stamp is included in the next task's commit.

Status rules:
- Containers (tasks with children) are never started directly; their status is derived: done when every
  descendant is closed and at least one is done, deferred when all are deferred, otherwise pending (or
  in_progress when a descendant is in progress). Reopening or adding a child reopens the container.
- A dependency is satisfied only by `done` work (a deferred dependency does not unblock dependents).
- Top-level epics without children must be split (import subtasks) before work starts.
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


def descendants(data: dict, task_id: str) -> list:
    out = []
    for k in children(data, task_id):
        out.append(k)
        out.extend(descendants(data, k["id"]))
    return out


def leaves(data: dict, task_id: str) -> list:
    kids = children(data, task_id)
    if not kids:
        return [index(data)[task_id]]
    out = []
    for k in kids:
        out.extend(leaves(data, k["id"]))
    return out


def ancestors(data: dict, task_id: str) -> list:
    idx = index(data)
    out = []
    p = idx[task_id]["parent"]
    while p is not None and p in idx:
        out.append(p)
        p = idx[p]["parent"]
    return out


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
    if errors:
        return errors
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
    if errors:
        return errors
    for t in data["tasks"]:
        tid = t["id"]
        anc = set(ancestors(data, tid))
        desc = {d["id"] for d in descendants(data, tid)}
        for dep in t.get("dependencies", []):
            if dep not in idx:
                errors.append(f"{tid}: dependency {dep} does not exist")
            elif dep == tid:
                errors.append(f"{tid}: depends on itself")
            elif dep in anc:
                errors.append(f"{tid}: depends on its own ancestor {dep} (deadlock)")
            elif dep in desc:
                errors.append(f"{tid}: depends on its own descendant {dep} (deadlock)")
    if errors:
        return errors
    # Cycle detection on the leaf graph: a leaf waits for its own deps and its ancestors' deps; a dependency
    # on a container means every leaf of that container.
    leaf_ids = [t["id"] for t in data["tasks"] if not children(data, t["id"])]
    edges: dict = {}
    for lid in leaf_ids:
        waits = set()
        for owner in [lid] + ancestors(data, lid):
            for dep in idx[owner]["dependencies"]:
                waits.update(x["id"] for x in leaves(data, dep))
        edges[lid] = waits
    state: dict = {}

    def visit(node: str, stack: list) -> None:
        if state.get(node) == 2:
            return
        if state.get(node) == 1:
            errors.append("dependency cycle: " + " -> ".join(stack + [node]))
            return
        state[node] = 1
        for nxt in sorted(edges.get(node, ())):
            visit(nxt, stack + [node])
            if len(errors) > 20:
                return
        state[node] = 2

    for lid in leaf_ids:
        visit(lid, [])
    return errors


def deps_done(data: dict, t: dict) -> bool:
    idx = index(data)
    for dep in t["dependencies"]:
        if any(leaf["status"] != "done" and leaf["status"] != "deferred" for leaf in leaves(data, dep)):
            return False
        # A deferred dependency does not unblock dependents; a container needs at least one done leaf.
        if not any(leaf["status"] == "done" for leaf in leaves(data, dep)):
            return False
        if not children(data, dep) and idx[dep]["status"] != "done":
            return False
    return True


def ancestors_ok(data: dict, t: dict) -> bool:
    idx = index(data)
    for p in ancestors(data, t["id"]):
        pt = idx[p]
        if pt["status"] in ("blocked", "deferred"):
            return False
        if not deps_done(data, pt):
            return False
    return True


def ready(data: dict) -> list:
    out = []
    for t in data["tasks"]:
        if t["status"] != "pending" or children(data, t["id"]):
            continue
        if t["parent"] is None:
            continue  # unsplit epic: must be split before work starts
        if deps_done(data, t) and ancestors_ok(data, t):
            out.append(t)
    out.sort(key=lambda t: (t["priority"], id_key(t["id"])))
    return out


def unsplit_epics(data: dict) -> list:
    return [
        t
        for t in children(data, None)
        if not children(data, t["id"]) and t["status"] == "pending" and deps_done(data, t)
    ]


def sync_containers(data: dict) -> list:
    """Recompute container statuses from their children (both directions). Returns changed (id, old, new)."""
    changed = []
    for t in sorted(data["tasks"], key=lambda t: -len(t["id"].split("."))):  # deepest first
        kids = children(data, t["id"])
        if not kids:
            continue
        st = [k["status"] for k in kids]
        if all(s in CLOSED for s in st):
            new = "done" if any(s == "done" for s in st) else "deferred"
        elif any(s == "in_progress" for s in st):
            new = "in_progress"
        elif t["status"] == "blocked":
            new = "blocked"
        else:
            new = "pending"
        if new != t["status"]:
            changed.append((t["id"], t["status"], new))
            t["status"] = new
            if new == "done":
                t["commit"] = t["commit"] or "children"
                if not t["notes"]:
                    t["notes"] = "Closed automatically when all children closed."
            elif new == "deferred" and not t["notes"]:
                t["notes"] = "All children deferred."
            elif t["commit"] == "children":
                t["commit"] = ""
    return changed


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
    lines.append("**Totals:** " + ", ".join(f"{k}: {v}" for k, v in counts.items()) + f" (of {len(data['tasks'])})")
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
            lines.append(
                f"{indent}- {BOX[task['status']]} **{task['id']}** {task['title']} (P{task['priority']}){suffix}"
            )
        for k in children(data, task["id"]):
            emit(k, depth + 1)
        if depth == 0:
            lines.append("")

    for top in children(data, None):
        emit(top, 0)
    ROADMAP.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8", newline="\n")


HEADER_RE = re.compile(r"<!-- status:begin -->.*?<!-- status:end -->", re.S)


def progress_text(data: dict, log_line: str | None) -> str:
    text = PROGRESS.read_text(encoding="utf-8") if PROGRESS.exists() else ""
    current = [t for t in data["tasks"] if t["status"] == "in_progress" and not children(data, t["id"])]
    done = [t for t in data["tasks"] if t["status"] == "done" and not children(data, t["id"])]
    last = data.get("last_completed")
    nxt = ready(data)
    block = "\n".join([
        "<!-- status:begin -->",
        f"- **Updated:** {now()}",
        "- **Current task:** " + (", ".join(f"{t['id']} {t['title']}" for t in current) if current else "none"),
        "- **Last completed:** " + (last if last else "none"),
        "- **Next task:** " + (f"{nxt[0]['id']} {nxt[0]['title']}" if nxt else "none ready (see `tasks.py next`)"),
        f"- **Done leaf tasks:** {len(done)}",
        "<!-- status:end -->",
    ])
    if HEADER_RE.search(text):
        text = HEADER_RE.sub(lambda _m: block, text)
    else:
        text = (
            "# Talewright Progress\n\n"
            + block
            + "\n\n## Log\n\n| Time | Task | What | Commit |\n|---|---|---|---|\n"
            + text
        )
    if log_line:
        text = text.rstrip("\n") + "\n" + log_line + "\n"
    return text


def write_all(data: dict, log_line: str | None) -> None:
    """Compute every output first, then write, so a failure cannot leave the files out of sync."""
    errors = validate(data)
    if errors:
        sys.exit("refusing to write, validation failed:\n  " + "\n  ".join(errors[:30]))
    progress = progress_text(data, log_line)
    save(data)
    render(data)
    PROGRESS.write_text(progress, encoding="utf-8", newline="\n")


def md_cell(s: str) -> str:
    return s.replace("|", "\\|").replace("\n", " ")


def cmd_set_status(args, status: str) -> None:
    data = load()
    t = get(data, args.id)
    msg = args.message or ""
    if children(data, t["id"]):
        sys.exit("error: containers derive their status from children; act on a leaf task")
    if status in ("blocked", "deferred") and not msg:
        sys.exit(f"error: {status} needs -m with the exact reason")
    if status == "in_progress":
        if t["parent"] is None:
            sys.exit("error: this epic has no subtasks; split it first (tasks.py import)")
        if not deps_done(data, t) or not ancestors_ok(data, t):
            sys.exit(
                "error: dependencies are not done: "
                + ", ".join(t["dependencies"] + [a for a in ancestors(data, t["id"])])
            )
    t["status"] = status
    if status == "done":
        t["commit"] = "pending"
        data["last_completed"] = f"{t['id']} {t['title']}"
    if status == "pending" and t["commit"] == "pending":
        t["commit"] = ""
    if msg and status in ("blocked", "deferred", "pending"):
        t["notes"] = (msg + ("\n" + t["notes"] if t["notes"] else "")).strip()
    changed = sync_containers(data)
    verb = {
        "in_progress": "started",
        "done": "done",
        "blocked": "BLOCKED",
        "deferred": "deferred",
        "pending": "reopened",
    }[status]
    what = f"{verb}: {t['title']}" + (f" — {msg}" if msg else "")
    if changed:
        what += " (containers: " + ", ".join(f"{i} {a}->{b}" for i, a, b in changed) + ")"
    commit = "pending" if status == "done" else ""
    write_all(data, f"| {now()} | {t['id']} | {md_cell(what)} | {commit} |")
    print(f"{t['id']} -> {status}")
    for i, a, b in changed:
        print(f"container {i}: {a} -> {b}")


def cmd_stamp(_args) -> None:
    data = load()
    changed = []
    missing = []
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
        else:
            missing.append(t["id"])
    if changed:
        progress = PROGRESS.read_text(encoding="utf-8")
        for tid, h in changed:
            progress = re.sub(
                rf"(\| {re.escape(tid)} \| [^\n]*\| )pending \|", lambda m, h=h: m.group(1) + h + " |", progress
            )
        save(data)
        render(data)
        PROGRESS.write_text(progress, encoding="utf-8", newline="\n")
    for tid, h in changed:
        print(f"{tid}: {h}")
    for tid in missing:
        print(f"WARNING {tid}: marked done but no commit '{tid}: ...' exists yet — commit it now")
    if not changed and not missing:
        print("nothing to stamp")


def cmd_import(args) -> None:
    data = load()
    idx = index(data)
    new = json.loads(Path(args.file).read_text(encoding="utf-8"))
    added, updated = 0, 0
    for t in new:
        if t["id"] in idx:
            # Update only the fields present in the file; status/commit/notes are kept unless given.
            idx[t["id"]].update(t)
            updated += 1
        else:
            for key, default in (
                ("dependencies", []),
                ("status", "pending"),
                ("priority", 2),
                ("files", []),
                ("commit", ""),
                ("notes", ""),
            ):
                t.setdefault(key, default)
            if "parent" not in t:
                t["parent"] = t["id"].rsplit(".", 1)[0] if "." in t["id"] else None
            data["tasks"].append(t)
            idx[t["id"]] = t
            added += 1
    changed = sync_containers(data)
    what = f"imported {added} new / {updated} updated tasks"
    if changed:
        what += " (containers: " + ", ".join(f"{i} {a}->{b}" for i, a, b in changed) + ")"
    write_all(data, f"| {now()} | - | {md_cell(what)} | |")
    print(what)


def git_lines(*cmd: str) -> list:
    out = subprocess.run(["git", *cmd], cwd=ROOT, capture_output=True, text=True, check=False).stdout
    return [ln for ln in out.splitlines() if ln.strip()]


def cmd_doctor(_args) -> None:
    data = load()
    print("== in-progress leaf tasks (finish or revert each before new work) ==")
    for t in data["tasks"]:
        if t["status"] == "in_progress" and not children(data, t["id"]):
            files = ", ".join(t["files"]) or "(no files recorded)"
            print(f"  {t['id']} {t['title']}\n      files: {files}")
    print("== done tasks whose commit is not stamped ==")
    for t in data["tasks"]:
        if t["commit"] == "pending":
            exists = bool(git_lines("log", "-n", "1", "--format=%h", "--grep", f"^{re.escape(t['id'])}: "))
            print(
                f"  {t['id']}: {'commit exists -> run tasks.py stamp' if exists else 'NO COMMIT YET -> commit its files now'}"
            )
    print("== uncommitted paths (git status --porcelain) ==")
    for ln in git_lines("status", "--porcelain"):
        print("  " + ln)
    epics = unsplit_epics(data)
    if epics:
        print("== epics ready to split into subtasks ==")
        for t in epics:
            print(f"  {t['id']} {t['title']}")
    errors = validate(data)
    print("== validate ==\n  " + ("ok" if not errors else "\n  ".join(errors)))


def cmd_next(args) -> None:
    data = load()
    current = [t for t in data["tasks"] if t["status"] == "in_progress" and not children(data, t["id"])]
    for t in current:
        print(f"{t['id']}\tP{t['priority']}\t{t['title']}\t[IN PROGRESS]")
    nxt = ready(data)
    for t in nxt[: args.n]:
        print(f"{t['id']}\tP{t['priority']}\t{t['title']}")
    for t in unsplit_epics(data):
        print(f"{t['id']}\tP{t['priority']}\t{t['title']}\t[EPIC: split into subtasks first]")
    if not nxt and not current:
        blocked = [t for t in data["tasks"] if t["status"] == "blocked"]
        print(
            "nothing ready: "
            + (f"{len(blocked)} blocked task(s); " if blocked else "")
            + "check dependencies with `tasks.py doctor`"
        )


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    for name in ("validate", "render", "stats", "stamp", "doctor"):
        sub.add_parser(name)
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
        changed = sync_containers(data)
        write_all(data, None)
        print("rendered" + (f" (containers synced: {changed})" if changed else ""))
    elif args.cmd == "stats":
        data = load()
        counts = {s: 0 for s in STATUSES}
        for t in data["tasks"]:
            counts[t["status"]] += 1
        print(json.dumps(counts))
    elif args.cmd == "next":
        cmd_next(args)
    elif args.cmd == "doctor":
        cmd_doctor(args)
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
        PROGRESS.write_text(
            progress_text(data, f"| {now()} | {args.id} | {md_cell(args.message)} | |"), encoding="utf-8", newline="\n"
        )
        print("logged")


if __name__ == "__main__":
    main()
