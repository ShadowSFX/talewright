#!/usr/bin/env python3
"""Command-line client for the Talewright Claude bridge (MCP over Streamable HTTP).

The editor writes <project>/.godot/claude_bridge/connection.json (url, port, token_file, pid). This client
takes the URL from it (loopback only) or from --url, and the token from the per-user editor config path
(or --token-file, e.g. for self-contained editors); it sends JSON-RPC requests, prints results and saves
image results as PNG files so they can be opened (e.g. by Claude Code's Read tool).

Usage (from the repository root):
    python talewright/tools/bridge.py [--project DIR] status
    python talewright/tools/bridge.py [--project DIR] list
    python talewright/tools/bridge.py [--project DIR] call <tool> [--args '{"k": 1}'] [key=value ...]
    python talewright/tools/bridge.py [--project DIR] raw '{"jsonrpc":"2.0","id":1,"method":"ping"}'
    python talewright/tools/bridge.py launch --project DIR [--headless] [--wait 90]
    python talewright/tools/bridge.py [--project DIR] wait [--timeout 90]
Options: --out-dir DIR (images, default talewright/build_logs/bridge), --timeout SEC (HTTP, default 60).
key=value arguments are parsed as JSON when possible (numbers, true/false, lists), else as strings.
Exit codes: 0 ok, 1 tool returned isError, 2 transport/protocol error.
"""

from __future__ import annotations

import argparse
import base64
import io
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
EDITOR = ROOT / "bin" / "godot.windows.editor.x86_64.exe"
DEFAULT_OUT = ROOT / "talewright" / "build_logs" / "bridge"
PROTOCOL = "2025-06-18"


class BridgeError(Exception):
    pass


def find_connection(project: str | None) -> Path:
    candidates = []
    if project:
        candidates.append(Path(project))
    else:
        cwd = Path.cwd()
        candidates += [cwd, *cwd.parents]
    for base in candidates:
        p = base / ".godot" / "claude_bridge" / "connection.json"
        if p.is_file():
            return p
    raise BridgeError("connection.json not found; is the editor running with the bridge? (use --project DIR)")


LOOPBACK_PREFIXES = ("http://127.0.0.1:", "http://localhost:", "http://[::1]:")


def default_token_path() -> Path:
    """The editor writes the token to <editor config dir>/claude_bridge/token (claude_bridge_plugin.cpp)."""
    if os.name == "nt":
        return Path(os.environ.get("APPDATA", "")) / "Godot" / "claude_bridge" / "token"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / "Godot" / "claude_bridge" / "token"
    return Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")) / "godot" / "claude_bridge" / "token"


def load_endpoint(args) -> tuple[Any, ...]:
    # A connection.json inside a project folder could be crafted by whoever shipped the project: only loopback URLs
    # are accepted and the token is always read from the per-user editor config path (or an explicit --token-file).
    token_file = Path(args.token_file) if args.token_file else default_token_path()
    if args.url:
        url = args.url
    else:
        info = json.loads(find_connection(args.project).read_text(encoding="utf-8"))
        url = str(info.get("url", ""))
    if not url.startswith(LOOPBACK_PREFIXES):
        raise BridgeError(f"refusing non-loopback bridge URL {url!r}")
    if not token_file.is_file():
        raise BridgeError(f"token file not found: {token_file} (start the editor once, or pass --token-file)")
    return url, token_file.read_text(encoding="utf-8").strip()


# Never route bridge traffic (which carries the token) through HTTP proxies from the environment or registry.
_OPENER = urllib.request.build_opener(urllib.request.ProxyHandler({}))


_next_id = 1


def rpc(
    url: str, token: str, method: str, params: dict[str, Any] | None = None, timeout: float = 60.0
) -> dict[str, Any]:
    global _next_id
    body = {"jsonrpc": "2.0", "id": _next_id, "method": method}
    _next_id += 1
    if params is not None:
        body["params"] = params
    data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        method="POST",
        headers={
            "Content-Type": "application/json",
            "Accept": "application/json, text/event-stream",
            "Authorization": f"Bearer {token}",
            "MCP-Protocol-Version": PROTOCOL,
        },
    )
    try:
        with _OPENER.open(req, timeout=timeout) as resp:
            payload = resp.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        raise BridgeError(f"HTTP {e.code}: {e.read().decode('utf-8', 'replace')}") from e
    except (urllib.error.URLError, TimeoutError, ConnectionError) as e:
        raise BridgeError(f"cannot reach the bridge at {url}: {e}") from e
    msg = json.loads(payload)
    if "error" in msg:
        raise BridgeError(f"JSON-RPC error {msg['error'].get('code')}: {msg['error'].get('message')}")
    result: dict[str, Any] = msg["result"]
    return result


def parse_kv(pairs: list[Any]) -> dict[str, Any]:
    out = {}
    for p in pairs:
        if "=" not in p:
            raise BridgeError(f"expected key=value, got {p!r}")
        k, v = p.split("=", 1)
        try:
            out[k] = json.loads(v)
        except json.JSONDecodeError:
            out[k] = v
    return out


def print_result(result: dict[str, Any], out_dir: Path, tool: str) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    for i, block in enumerate(result.get("content", [])):
        kind = block.get("type")
        if kind == "text":
            print(block.get("text", ""))
        elif kind == "image":
            ext = "png" if block.get("mimeType") == "image/png" else "bin"
            path = out_dir / f"{time.strftime('%Y%m%d-%H%M%S')}-{tool}-{i}.{ext}"
            path.write_bytes(base64.b64decode(block.get("data", "")))
            print(f"[image saved] {path}")
        else:
            print(json.dumps(block, indent=2))
    return 1 if result.get("isError") else 0


def cmd_launch(args) -> int:
    if not args.project:
        raise BridgeError("launch requires --project DIR")
    conn = Path(args.project) / ".godot" / "claude_bridge" / "connection.json"
    if conn.exists():
        conn.unlink()  # Stale file from a previous session; the editor rewrites it when the bridge starts.
    cmd = [str(EDITOR), "--editor", "--path", str(Path(args.project).resolve())]
    if args.headless:
        cmd.insert(1, "--headless")
    log = (ROOT / "talewright" / "build_logs" / "bridge_editor.log").open("w", encoding="utf-8")
    flags = getattr(subprocess, "DETACHED_PROCESS", 0) | getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)
    proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, creationflags=flags)
    print(f"launched editor pid {proc.pid}: {' '.join(cmd)}")
    args.timeout = args.wait
    return cmd_wait(args)


def cmd_wait(args) -> int:
    deadline = time.monotonic() + args.timeout
    last = ""
    while time.monotonic() < deadline:
        try:
            url, token = load_endpoint(args)
            result = rpc(url, token, "tools/call", {"name": "bridge_status", "arguments": {}}, timeout=5)
            print_result(result, Path(args.out_dir), "bridge_status")
            return 0
        except BridgeError as e:
            last = str(e)
        time.sleep(1.0)
    raise BridgeError(f"bridge not ready after {args.timeout}s: {last}")


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    # Global options are accepted before or after the subcommand.
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "--project", default=argparse.SUPPRESS, help="project dir with .godot/claude_bridge/connection.json"
    )
    common.add_argument("--url", default=argparse.SUPPRESS)
    common.add_argument("--token-file", default=argparse.SUPPRESS)
    common.add_argument("--out-dir", default=argparse.SUPPRESS)
    common.add_argument("--timeout", type=float, default=argparse.SUPPRESS)
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter, parents=[common]
    )
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status", parents=[common])
    sub.add_parser("list", parents=[common])
    c = sub.add_parser("call", parents=[common])
    c.add_argument("tool")
    c.add_argument("--args", default="")
    c.add_argument("kv", nargs="*")
    r = sub.add_parser("raw", parents=[common])
    r.add_argument("json")
    la = sub.add_parser("launch", parents=[common])
    la.add_argument("--headless", action="store_true")
    la.add_argument("--wait", type=float, default=120.0)
    sub.add_parser("wait", parents=[common])
    args = ap.parse_args()
    for name, default in (
        ("project", None),
        ("url", None),
        ("token_file", None),
        ("out_dir", str(DEFAULT_OUT)),
        ("timeout", 60.0),
    ):
        if not hasattr(args, name):
            setattr(args, name, default)

    try:
        if args.cmd == "launch":
            return cmd_launch(args)
        if args.cmd == "wait":
            return cmd_wait(args)
        url, token = load_endpoint(args)
        if args.cmd == "status":
            return print_result(
                rpc(url, token, "tools/call", {"name": "bridge_status", "arguments": {}}, args.timeout),
                Path(args.out_dir),
                "bridge_status",
            )
        if args.cmd == "list":
            result = rpc(url, token, "tools/list", {}, args.timeout)
            for t in result.get("tools", []):
                ro = "ro" if t.get("annotations", {}).get("readOnlyHint") else "rw"
                print(f"{t['name']:28} [{ro}] {t.get('description', '')}")
            return 0
        if args.cmd == "call":
            arguments = json.loads(args.args) if args.args else {}
            arguments.update(parse_kv(args.kv))
            result = rpc(url, token, "tools/call", {"name": args.tool, "arguments": arguments}, args.timeout)
            return print_result(result, Path(args.out_dir), args.tool)
        if args.cmd == "raw":
            body = json.loads(args.json)
            print(json.dumps(rpc(url, token, body["method"], body.get("params"), args.timeout), indent=2))
            return 0
    except BridgeError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    sys.exit(main())
