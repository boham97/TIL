#!/usr/bin/env python3
"""네트워크 부하 생성기 - nethogs / iftop / tcpdump 로 send/receive 트래픽을 확인하기 위한 도구.

모드
  download  인터넷(speed.cloudflare.com)에서 데이터를 받아 RECEIVE 부하 생성
  upload    인터넷(speed.cloudflare.com)으로 데이터를 보내 SEND 부하 생성
  both      download + upload 동시
  server    LAN 테스트용 TCP 서버 (다른 PC가 client 로 접속)
  client    LAN 테스트용 TCP 클라이언트 (--dir send|recv|both)

주의: 자기 자신의 IP(127.0.0.1, 또는 enp2s0 IP)로 보내는 트래픽은 lo 인터페이스로만 흐르므로
      `nethogs enp2s0` 에는 보이지 않는다. 인터넷 모드나 다른 PC와의 server/client 모드를 쓸 것.
"""
import argparse
import http.client
import os
import socket
import sys
import threading
import time

CHUNK = 64 * 1024
CF_HOST = "speed.cloudflare.com"
DL_BYTES = 50 * 1024 * 1024  # 요청 1회당 다운로드 크기
UL_BYTES = 10 * 1024 * 1024  # 요청 1회당 업로드 크기
PAYLOAD = os.urandom(CHUNK)


class Stats:
    def __init__(self):
        self.lock = threading.Lock()
        self.sent = 0
        self.recv = 0

    def add(self, sent=0, recv=0):
        with self.lock:
            self.sent += sent
            self.recv += recv

    def snapshot(self):
        with self.lock:
            return self.sent, self.recv


class Throttle:
    """초당 bps 바이트를 넘지 않도록 sleep. bps=0 이면 무제한."""

    def __init__(self, bps):
        self.bps = bps
        self.start = time.monotonic()
        self.total = 0

    def wait(self, n):
        if not self.bps:
            return
        self.total += n
        ahead = self.total / self.bps - (time.monotonic() - self.start)
        if ahead > 0:
            time.sleep(ahead)


# ---------------------------------------------------------------- 인터넷 모드

def download_worker(stats, stop, bps):
    th = Throttle(bps)
    while not stop.is_set():
        try:
            conn = http.client.HTTPSConnection(CF_HOST, timeout=10)
            conn.request("GET", f"/__down?bytes={DL_BYTES}")
            resp = conn.getresponse()
            while not stop.is_set():
                data = resp.read(CHUNK)
                if not data:
                    break
                stats.add(recv=len(data))
                th.wait(len(data))
            conn.close()
        except OSError as e:
            print(f"\n[download] 오류: {e}", file=sys.stderr)
            stop.wait(1)


def upload_worker(stats, stop, bps):
    th = Throttle(bps)
    while not stop.is_set():
        try:
            conn = http.client.HTTPSConnection(CF_HOST, timeout=10)
            conn.putrequest("POST", "/__up")
            conn.putheader("Content-Type", "application/octet-stream")
            conn.putheader("Content-Length", str(UL_BYTES))
            conn.endheaders()
            left = UL_BYTES
            while left > 0 and not stop.is_set():
                n = min(CHUNK, left)
                conn.send(PAYLOAD[:n])
                stats.add(sent=n)
                th.wait(n)
                left -= n
            if left == 0:
                conn.getresponse().read()
            conn.close()
        except OSError as e:
            print(f"\n[upload] 오류: {e}", file=sys.stderr)
            stop.wait(1)


# ---------------------------------------------------------------- LAN 모드

def sock_sender(sock, stats, stop, bps):
    th = Throttle(bps)
    try:
        while not stop.is_set():
            sock.sendall(PAYLOAD)
            stats.add(sent=CHUNK)
            th.wait(CHUNK)
    except OSError:
        pass


def sock_receiver(sock, stats, stop, bps):
    th = Throttle(bps)
    try:
        while not stop.is_set():
            data = sock.recv(CHUNK)
            if not data:
                break
            stats.add(recv=len(data))
            th.wait(len(data))
    except OSError:
        pass


def run_duplex(sock, direction, stats, stop, bps):
    """direction 은 이 소켓 입장에서 send / recv / both."""
    threads = []
    if direction in ("send", "both"):
        threads.append(threading.Thread(target=sock_sender, args=(sock, stats, stop, bps), daemon=True))
    if direction in ("recv", "both"):
        threads.append(threading.Thread(target=sock_receiver, args=(sock, stats, stop, bps), daemon=True))
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    sock.close()


def server_main(args, stats, stop):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", args.port))
    srv.listen()
    srv.settimeout(1)
    print(f"[server] 0.0.0.0:{args.port} 에서 대기 중 (방화벽: firewall-cmd --add-port={args.port}/tcp)")
    # 클라이언트가 보낸 첫 바이트가 클라이언트 입장의 방향 → 서버는 반대로 동작
    opposite = {b"S": "recv", b"R": "send", b"B": "both"}
    while not stop.is_set():
        try:
            conn, addr = srv.accept()
        except socket.timeout:
            continue
        mode = conn.recv(1)
        direction = opposite.get(mode)
        if not direction:
            conn.close()
            continue
        print(f"\n[server] {addr[0]}:{addr[1]} 접속 → 서버는 {direction}")
        threading.Thread(target=run_duplex, args=(conn, direction, stats, stop, args.bps), daemon=True).start()
    srv.close()


def client_worker(args, stats, stop):
    sock = socket.create_connection((args.host, args.port), timeout=10)
    sock.settimeout(None)
    sock.sendall({"send": b"S", "recv": b"R", "both": b"B"}[args.dir])
    run_duplex(sock, args.dir, stats, stop, args.bps)


# ---------------------------------------------------------------- 공통

def fmt(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:7.1f} {unit}"
        n /= 1024
    return f"{n:7.1f} TB"


def report(stats, stop, duration):
    start = time.monotonic()
    prev_s, prev_r = 0, 0
    try:
        while not stop.is_set():
            time.sleep(1)
            s, r = stats.snapshot()
            elapsed = time.monotonic() - start
            print(f"\r[{elapsed:6.0f}s] SEND {fmt(s - prev_s)}/s (누적 {fmt(s)})  "
                  f"RECV {fmt(r - prev_r)}/s (누적 {fmt(r)})", end="", flush=True)
            prev_s, prev_r = s, r
            if duration and elapsed >= duration:
                break
    except KeyboardInterrupt:
        pass
    stop.set()
    print("\n종료합니다.")


def main():
    p = argparse.ArgumentParser(description="네트워크 부하 생성기 (nethogs 확인용)")
    p.add_argument("mode", choices=["download", "upload", "both", "server", "client"])
    p.add_argument("host", nargs="?", help="client 모드에서 접속할 서버 IP")
    p.add_argument("-d", "--duration", type=int, default=60, help="실행 시간(초), 0=Ctrl+C 까지 (기본 60)")
    p.add_argument("-r", "--rate", type=float, default=1, help="연결(스레드)당 최대 속도 MB/s, 0=무제한 (기본 1)")
    p.add_argument("-t", "--threads", type=int, default=1, help="방향별 동시 연결 수")
    p.add_argument("-p", "--port", type=int, default=5201, help="server/client 포트")
    p.add_argument("--dir", choices=["send", "recv", "both"], default="both", help="client 모드 방향")
    args = p.parse_args()
    args.bps = int(args.rate * 1024 * 1024)

    if args.mode == "client" and not args.host:
        p.error("client 모드에는 서버 IP 가 필요합니다. 예: netload.py client 192.168.0.20")

    stats = Stats()
    stop = threading.Event()
    targets = []
    if args.mode in ("download", "both"):
        targets += [(download_worker, (stats, stop, args.bps))] * args.threads
    if args.mode in ("upload", "both"):
        targets += [(upload_worker, (stats, stop, args.bps))] * args.threads
    if args.mode == "server":
        targets.append((server_main, (args, stats, stop)))
    if args.mode == "client":
        targets += [(client_worker, (args, stats, stop))] * args.threads

    print(f"PID {os.getpid()} - nethogs 에서 'python3' 프로세스로 표시됩니다. Ctrl+C 로 종료.")
    for fn, fargs in targets:
        threading.Thread(target=fn, args=fargs, daemon=True).start()
    report(stats, stop, args.duration)


if __name__ == "__main__":
    main()
