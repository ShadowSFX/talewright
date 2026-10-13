#!/usr/bin/env python3
"""Robustness and latency checks for a running Claude bridge (task 4.11).

Usage: python talewright/tools/bridge_stress.py --project <dir> [--calls 1000]
Checks:
  1. latency: sequential tools/call bridge_status over one keep-alive connection (median, p95);
  2. concurrency: 8 threads x 50 calls, all must succeed;
  3. slow client: a connection that sends half a request and stalls must not delay other clients and must get
     408 / be closed after the request timeout;
  4. stalled reader: a client that never reads a ~3 MB response must not block other clients and must be dropped
     after the server's write timeout;
  5. connection flood: more than max_connections sockets -> extra ones get 503, normal service resumes.
Exit code 1 if any check fails.
"""

from __future__ import annotations

import argparse
import http.client
import io
import json
import socket
import statistics
import sys
import threading
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bridge  # noqa: E402


def make_conn(url: str) -> http.client.HTTPConnection:
    host_port = url.split("//", 1)[1].split("/", 1)[0]
    host, port = host_port.split(":")
    return http.client.HTTPConnection(host, int(port), timeout=30)


def call(conn: http.client.HTTPConnection, token: str, rid: int) -> dict[str, Any]:
    body = json.dumps({
        "jsonrpc": "2.0",
        "id": rid,
        "method": "tools/call",
        "params": {"name": "bridge_status", "arguments": {}},
    })
    conn.request(
        "POST", "/mcp", body=body, headers={"Content-Type": "application/json", "Authorization": f"Bearer {token}"}
    )
    resp = conn.getresponse()
    data = resp.read()
    if resp.status != 200:
        raise RuntimeError(f"HTTP {resp.status}: {data[:200]!r}")
    result: dict[str, Any] = json.loads(data)
    return result


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--project", required=True)
    ap.add_argument("--calls", type=int, default=1000)
    args = ap.parse_args()
    ns = argparse.Namespace(project=args.project, url=None, token_file=None)
    url, token = bridge.load_endpoint(ns)
    host_port = url.split("//", 1)[1].split("/", 1)[0]
    host, port = host_port.split(":")
    port = int(port)
    ok = True

    # 1. Latency.
    conn = make_conn(url)
    lat = []
    for i in range(args.calls):
        t = time.perf_counter()
        call(conn, token, i)
        lat.append((time.perf_counter() - t) * 1000)
    conn.close()
    med, p95 = statistics.median(lat), sorted(lat)[int(len(lat) * 0.95) - 1]
    print(
        f"[{'PASS' if med < 50 else 'FAIL'}] latency over {args.calls} calls: median {med:.1f} ms, p95 {p95:.1f} ms, max {max(lat):.1f} ms"
    )
    ok &= med < 50

    # 2. Concurrency.
    errors = []

    def worker(n: int) -> None:
        try:
            c = make_conn(url)
            for i in range(50):
                call(c, token, n * 1000 + i)
            c.close()
        except Exception as e:  # noqa: BLE001
            errors.append(str(e))

    threads = [threading.Thread(target=worker, args=(n,)) for n in range(8)]
    t0 = time.perf_counter()
    for th in threads:
        th.start()
    for th in threads:
        th.join()
    print(
        f"[{'PASS' if not errors else 'FAIL'}] concurrency 8x50 in {time.perf_counter() - t0:.1f}s, errors: {errors[:3]}"
    )
    ok &= not errors

    # 3. Slow client (half a request) must not delay others.
    slow = socket.create_connection((host, port))
    slow.sendall(b"POST /mcp HTTP/1.1\r\nHost: 127.0.0.1:" + str(port).encode() + b"\r\nContent-Le")
    t = time.perf_counter()
    c = make_conn(url)
    call(c, token, 1)
    c.close()
    d = (time.perf_counter() - t) * 1000
    print(f"[{'PASS' if d < 1000 else 'FAIL'}] call while a slow client stalls: {d:.1f} ms")
    ok &= d < 1000
    slow.settimeout(25)
    try:
        got = slow.recv(4096)
        timed_out = got.startswith(b"HTTP/1.1 408") or got == b""
    except TimeoutError:
        timed_out = False
    print(f"[{'PASS' if timed_out else 'FAIL'}] slow client receives 408/close after the request timeout")
    ok &= timed_out
    slow.close()

    # 4. Stalled reader: request a ~3 MB response (larger than the socket buffers) and never read it.
    big = Path(args.project) / ".godot" / "claude_bridge" / "stress_big.txt"
    big.write_text("x" * (3 * 1024 * 1024), encoding="ascii")
    stalled = socket.create_connection((host, port))
    stalled.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
    body = json.dumps({
        "jsonrpc": "2.0",
        "id": 9,
        "method": "tools/call",
        "params": {
            "name": "project_read_file",
            "arguments": {"path": "res://.godot/claude_bridge/stress_big.txt", "max_bytes": 4194304},
        },
    }).encode()
    stalled.sendall(
        b"POST /mcp HTTP/1.1\r\nHost: 127.0.0.1:"
        + str(port).encode()
        + b"\r\nContent-Type: application/json\r\nAuthorization: Bearer "
        + token.encode()
        + b"\r\nContent-Length: "
        + str(len(body)).encode()
        + b"\r\n\r\n"
        + body
    )
    t = time.perf_counter()
    c = make_conn(url)
    call(c, token, 2)
    c.close()
    d = (time.perf_counter() - t) * 1000
    print(f"[{'PASS' if d < 1000 else 'FAIL'}] call while another client does not read a 3 MB response: {d:.1f} ms")
    ok &= d < 1000
    # The server must drop the stalled connection after its write timeout (30 s): drain and expect EOF.
    stalled.settimeout(60)
    received, closed = 0, False
    deadline = time.time() + 60
    time.sleep(35)
    try:
        while time.time() < deadline:
            chunk = stalled.recv(65536)
            if not chunk:
                closed = True
                break
            received += len(chunk)
    except (ConnectionResetError, ConnectionAbortedError):
        closed = True
    print(
        f"[{'PASS' if closed else 'FAIL'}] stalled reader dropped by the server's write timeout (received {received} bytes before close)"
    )
    ok &= closed
    stalled.close()
    big.unlink(missing_ok=True)

    # 5. Connection flood.
    socks = []
    rejected = 0
    for _ in range(24):
        s = socket.create_connection((host, port))
        socks.append(s)
    time.sleep(0.5)
    for s in socks:
        s.settimeout(0.5)
        try:
            data = s.recv(64)
            rejected += data.startswith(b"HTTP/1.1 503")
        except (TimeoutError, OSError):
            pass
    for s in socks:
        s.close()
    time.sleep(0.5)
    c = make_conn(url)
    call(c, token, 3)
    c.close()
    print(
        f"[{'PASS' if rejected >= 1 else 'FAIL'}] connection flood: {rejected} of 24 extra sockets rejected with 503; service resumed"
    )
    ok &= rejected >= 1

    print("ALL PASS" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
