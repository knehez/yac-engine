#!/usr/bin/env python3
"""Local browser GUI and UCI bridge for YaC-Engine."""

from __future__ import annotations

import argparse
import atexit
import json
import mimetypes
import os
import queue
import re
import sqlite3
import subprocess
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parents[1]
STATIC_DIR = Path(__file__).resolve().parent / "static"
START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
PUZZLE_INDEX = ROOT / "yac_lichess_import.sqlite3"


def validate_fen(fen: str) -> str:
    fields = fen.strip().split()
    if len(fields) != 6:
        raise ValueError("A FEN-nek 6 mezőt kell tartalmaznia.")
    ranks = fields[0].split("/")
    if len(ranks) != 8:
        raise ValueError("A FEN táblarészének 8 sort kell tartalmaznia.")
    pieces = "prnbqkPRNBQK"
    for rank in ranks:
        width = sum(int(token) if token.isdigit() else 1 for token in rank)
        if width != 8 or any(not (token.isdigit() or token in pieces) for token in rank):
            raise ValueError("Hibás mezőszám vagy bábu a FEN táblarészében.")
    if fields[0].count("K") != 1 or fields[0].count("k") != 1:
        raise ValueError("A pozícióban pontosan egy világos és egy sötét király szükséges.")
    if fields[1] not in ("w", "b"):
        raise ValueError("A lépő fél mezője csak 'w' vagy 'b' lehet.")
    if not re.fullmatch(r"-|K?Q?k?q?", fields[2]):
        raise ValueError("Hibás sáncolási mező.")
    if not re.fullmatch(r"-|[a-h][36]", fields[3]):
        raise ValueError("Hibás en passant mező.")
    if not fields[4].isdigit() or not fields[5].isdigit():
        raise ValueError("A FEN lépésszámlálóinak egész számnak kell lenniük.")
    return " ".join(fields)


def find_engine(explicit: str | None) -> Path:
    candidates = [
        explicit,
        os.environ.get("YACE_ENGINE_PATH"),
        ROOT / "build" / "src" / "Release" / "yace.exe",
        ROOT / "build" / "src" / "Debug" / "yace.exe",
        ROOT / "build" / "src" / "yace.exe",
        ROOT / "build" / "src" / "yace",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise FileNotFoundError(
        "A YaC-Engine futtatható állománya nem található. Fordítsd le Release módban, "
        "vagy add meg: python gui/server.py --engine <útvonal>"
    )


def random_imported_puzzle() -> dict:
    if not PUZZLE_INDEX.is_file():
        raise ValueError("Még nincs importált Lichess-feladvány.")
    connection = sqlite3.connect(PUZZLE_INDEX)
    try:
        row = connection.execute(
            "SELECT puzzle_id, fen, best_move FROM imported_samples ORDER BY RANDOM() LIMIT 1"
        ).fetchone()
    finally:
        connection.close()
    if not row:
        raise ValueError("Az importált Lichess-feladványok tára üres.")
    return {"puzzleId": row[0], "fen": row[1], "bestMove": row[2]}


class UciEngine:
    def __init__(self, executable: Path):
        self.executable = executable
        self._lock = threading.RLock()
        self._lines: queue.Queue[str] = queue.Queue()
        self._process = subprocess.Popen(
            [str(executable)],
            cwd=str(ROOT),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        self._reader = threading.Thread(target=self._read_stdout, daemon=True)
        self._reader.start()
        self._send("uci")
        self._wait_for(lambda line: line == "uciok", 5.0)
        self._send("setoption name Seed value 0")
        self._send("isready")
        self._wait_for(lambda line: line == "readyok", 5.0)

    def _read_stdout(self) -> None:
        assert self._process.stdout is not None
        for line in self._process.stdout:
            self._lines.put(line.strip())

    def _send(self, command: str) -> None:
        if self._process.poll() is not None:
            raise RuntimeError("A sakkmotor váratlanul leállt.")
        assert self._process.stdin is not None
        self._process.stdin.write(command + "\n")
        self._process.stdin.flush()

    def _wait_for(self, predicate, timeout: float) -> tuple[str, list[str]]:
        deadline = time.monotonic() + timeout
        seen: list[str] = []
        while time.monotonic() < deadline:
            try:
                line = self._lines.get(timeout=min(0.1, deadline - time.monotonic()))
            except queue.Empty:
                continue
            seen.append(line)
            if predicate(line):
                return line, seen
        raise TimeoutError(f"A motor nem válaszolt időben. Utolsó sorok: {seen[-5:]}")

    def _request(self, command: str, predicate, timeout: float = 5.0) -> tuple[str, list[str]]:
        self._send(command)
        return self._wait_for(predicate, timeout)

    def _set_position(self, fen: str, moves: list[str] | None = None) -> None:
        command = f"position fen {fen}"
        if moves:
            command += " moves " + " ".join(moves)
        self._send(command)

    def set_seed(self, seed: int) -> None:
        seed = max(0, min(int(seed), 2_147_483_647))
        with self._lock:
            self._send(f"setoption name Seed value {seed}")

    @staticmethod
    def _parse_policy_status(line: str) -> dict:
        match = re.match(
            r"policy enabled (\d+) learning (\d+) trained (\d+) samples (\d+) "
            r"updates (\d+) lastsample (\d+) lasttrained (\d+)",
            line,
        )
        return {
            "enabled": bool(int(match.group(1))) if match else False,
            "learning": bool(int(match.group(2))) if match else False,
            "trained": bool(int(match.group(3))) if match else False,
            "samples": int(match.group(4)) if match else 0,
            "updates": int(match.group(5)) if match else 0,
            "sampleCollected": bool(int(match.group(6))) if match else False,
            "trainedNow": bool(int(match.group(7))) if match else False,
        }

    def policy_status(self) -> dict:
        with self._lock:
            line, _ = self._request(
                "policystatus", lambda value: value.startswith("policy "), timeout=2.0
            )
            return self._parse_policy_status(line)

    def state(self, fen: str = START_FEN, moves: list[str] | None = None) -> dict:
        fen = validate_fen(fen)
        with self._lock:
            self._set_position(fen, moves)
            fen_line, position_output = self._request("fen", lambda line: line.startswith("fen "))
            invalid = next((line for line in position_output if line.startswith("Invalid move:")), None)
            if invalid:
                raise ValueError(invalid)
            moves_line, _ = self._request("legalmoves", lambda line: line.startswith("legalmoves"))
            return {
                "fen": fen_line[4:],
                "legalMoves": moves_line.split()[1:],
            }

    def play(self, fen: str, move: str) -> dict:
        with self._lock:
            current = self.state(fen)
            if move not in current["legalMoves"]:
                current["accepted"] = False
                return current
            result = self.state(current["fen"], [move])
            result["accepted"] = True
            return result

    def analyze(
        self,
        fen: str,
        iterations: int,
        exploration: float = 1.5,
        learned_policy: bool = False,
        policy_learning: bool = False,
    ) -> dict:
        fen = validate_fen(fen)
        iterations = max(1, min(iterations, 200_000))
        exploration = max(0.25, min(float(exploration), 5.0))
        with self._lock:
            self._send(f"setoption name Exploration value {round(exploration * 100)}")
            self._send(f"setoption name LearnedPolicy value {'true' if learned_policy else 'false'}")
            self._send(f"setoption name PolicyLearning value {'true' if policy_learning else 'false'}")
            self._set_position(fen)
            best_line, seen = self._request(
                f"go nodes {iterations}",
                lambda line: line.startswith("bestmove "),
                timeout=max(10.0, iterations / 2500.0),
            )
            info = next((line for line in reversed(seen) if line.startswith("info ")), "")
            score_match = re.search(r"score cp (-?\d+)", info)
            nodes_match = re.search(r"nodes (\d+)", info)
            pv_match = re.search(r"\bpv (\S+)", info)
            alternatives = []
            for line in seen:
                match = re.match(
                    r"info multipv (\d+) score cp (-?\d+) nodes (\d+)(?: string (\S+))? aiprior (\d+) pv (\S+)",
                    line,
                )
                if match:
                    annotation = match.group(4) or ""
                    alternatives.append({
                        "rank": int(match.group(1)),
                        "scoreCp": int(match.group(2)),
                        "visits": int(match.group(3)),
                        "aiPrior": int(match.group(5)) / 100.0,
                        "move": match.group(6),
                        "proven": annotation in ("proven", "mate-proven"),
                        "mateStatus": annotation.removeprefix("mate-") if annotation.startswith("mate-") else "none",
                    })
            policy_line, _ = self._request(
                "policystatus", lambda line: line.startswith("policy "), timeout=2.0
            )
            policy_status = self._parse_policy_status(policy_line)
            return {
                "bestMove": best_line.split(maxsplit=1)[1],
                "scoreCp": int(score_match.group(1)) if score_match else 0,
                "nodes": int(nodes_match.group(1)) if nodes_match else 0,
                "pv": pv_match.group(1) if pv_match else "",
                "alternatives": alternatives,
                "policy": policy_status,
            }

    def policy_action(self, action: str) -> dict:
        commands = {
            "train": "setoption name TrainPolicy",
            "clear": "setoption name ClearPolicy",
        }
        if action not in commands:
            raise ValueError("Ismeretlen policy művelet.")
        with self._lock:
            self._send(commands[action])
            self._wait_for(lambda line: line.startswith("info string policy "), 5.0)
            status, _ = self._request(
                "policystatus", lambda line: line.startswith("policy "), timeout=2.0
            )
            return {"status": status}

    def begin_policy_import(self) -> None:
        with self._lock:
            self._request(
                "policyimport begin",
                lambda line: line == "policyimport ready",
                timeout=5.0,
            )

    def add_policy_label(self, fen: str, best_move: str, confidence: float = 1.0) -> bool:
        fen = validate_fen(fen)
        confidence = max(0.0, min(float(confidence), 1.0))
        if not re.fullmatch(r"[a-h][1-8][a-h][1-8][qrbn]?", best_move):
            return False
        with self._lock:
            self._set_position(fen)
            line, _ = self._request(
                f"policylabel {best_move} {confidence:.6f}",
                lambda value: value.startswith("policylabel "),
                timeout=5.0,
            )
            return line.startswith("policylabel accepted ")

    def end_policy_import(self) -> dict:
        with self._lock:
            line, _ = self._request(
                "policyimport end",
                lambda value: value.startswith("policyimport complete "),
                timeout=30.0,
            )
        match = re.fullmatch(r"policyimport complete accepted (\d+) saved ([01])", line)
        if not match or match.group(2) != "1":
            raise RuntimeError("A külső tanítóminták mentése nem sikerült.")
        return {"accepted": int(match.group(1)), "saved": True}

    def evaluate(self, fen: str) -> dict:
        fen = validate_fen(fen)
        with self._lock:
            self._set_position(fen)
            line, _ = self._request("evaluate", lambda value: value.startswith("info score cp "))
            return {"scoreCp": int(line.rsplit(" ", 1)[1])}

    def close(self) -> None:
        if self._process.poll() is None:
            try:
                self._send("quit")
                self._process.wait(timeout=2)
            except (BrokenPipeError, TimeoutError, subprocess.TimeoutExpired):
                self._process.kill()


class GuiHandler(BaseHTTPRequestHandler):
    engine: UciEngine

    def log_message(self, fmt: str, *args) -> None:
        print(f"[gui] {self.address_string()} — {fmt % args}")

    def _json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _read_json(self) -> dict:
        length = int(self.headers.get("Content-Length", "0"))
        return json.loads(self.rfile.read(length) or b"{}")

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/api/health":
            self._json({"ok": True, "engine": str(self.engine.executable)})
            return
        if path == "/api/puzzle/random":
            try:
                self._json(random_imported_puzzle())
            except (ValueError, sqlite3.Error) as exc:
                self._json({"error": str(exc)}, 404)
            return
        filename = "index.html" if path == "/" else path.lstrip("/")
        target = (STATIC_DIR / filename).resolve()
        if STATIC_DIR.resolve() not in target.parents or not target.is_file():
            self.send_error(404)
            return
        content = target.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", mimetypes.guess_type(target.name)[0] or "application/octet-stream")
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def do_POST(self) -> None:
        try:
            data = self._read_json()
            if self.path == "/api/state":
                result = self.engine.state(data.get("fen") or START_FEN)
            elif self.path == "/api/move":
                result = self.engine.play(data["fen"], data["move"])
            elif self.path == "/api/analyze":
                result = self.engine.analyze(
                    data["fen"],
                    int(data.get("iterations", 2000)),
                    float(data.get("exploration", 1.5)),
                    bool(data.get("learnedPolicy", False)),
                    bool(data.get("policyLearning", False)),
                )
            elif self.path == "/api/policy":
                result = self.engine.policy_action(str(data.get("action", "")))
            elif self.path == "/api/evaluate":
                result = self.engine.evaluate(data["fen"])
            else:
                self._json({"error": "Ismeretlen végpont."}, 404)
                return
            self._json(result)
        except (KeyError, ValueError, RuntimeError, TimeoutError, json.JSONDecodeError) as exc:
            self._json({"error": str(exc)}, 400)


def main() -> None:
    parser = argparse.ArgumentParser(description="YaC-Engine helyi webes sakk GUI")
    parser.add_argument("--engine", help="A YaC-Engine futtatható állományának útvonala")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()

    engine = UciEngine(find_engine(args.engine))
    atexit.register(engine.close)
    GuiHandler.engine = engine
    server = ThreadingHTTPServer((args.host, args.port), GuiHandler)
    url = f"http://{args.host}:{args.port}"
    print(f"YaC Studio: {url}")
    print(f"UCI motor: {engine.executable}")
    if not args.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        engine.close()


if __name__ == "__main__":
    main()
