#!/usr/bin/env python3
"""Paired YaC-Control versus YaC-AI self-play benchmark."""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import random
import sqlite3
import sys
import time
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path

import chess
import chess.engine
import chess.pgn

try:
    from .server import ROOT, find_engine
except ImportError:  # Direct execution: python gui/selfplay_benchmark.py
    from server import ROOT, find_engine


BOOK_URL = (
    "https://raw.githubusercontent.com/official-stockfish/books/master/"
    "4mvs_%2B90_%2B99.epd.zip"
)
DEFAULT_DIR = ROOT / "build" / "selfplay"
DEFAULT_DATABASE = ROOT / "yac_selfplay.sqlite3"


@dataclass
class PlayedGame:
    result: str
    termination: str
    plies: int
    duration_ms: float
    moves: list[chess.Move]


def ensure_opening_book(directory: Path = DEFAULT_DIR) -> Path:
    directory.mkdir(parents=True, exist_ok=True)
    epd_path = directory / "4mvs_+90_+99.epd"
    if epd_path.is_file():
        return epd_path
    archive_path = directory / "4mvs_+90_+99.epd.zip"
    if not archive_path.is_file():
        print("Megnyitáskészlet letöltése…", flush=True)
        urllib.request.urlretrieve(BOOK_URL, archive_path)
    with zipfile.ZipFile(archive_path) as archive:
        members = [name for name in archive.namelist() if name.lower().endswith(".epd")]
        if len(members) != 1:
            raise ValueError("A megnyitásarchívum nem egyetlen EPD-fájlt tartalmaz.")
        with archive.open(members[0]) as source, epd_path.open("wb") as target:
            target.write(source.read())
    return epd_path


def load_openings(path: Path) -> list[chess.Board]:
    openings: list[chess.Board] = []
    with path.open("r", encoding="utf-8", errors="replace") as stream:
        for line in stream:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            try:
                fields = line.split()
                if len(fields) >= 6 and fields[4].isdigit() and fields[5].isdigit():
                    board = chess.Board(" ".join(fields[:6]))
                else:
                    board, _ = chess.Board.from_epd(line)
            except ValueError:
                continue
            if board.is_valid() and not board.is_game_over(claim_draw=True):
                openings.append(board)
    if not openings:
        raise ValueError("Nem található érvényes megnyitás az EPD-fájlban.")
    return openings


def _open_database(path: Path) -> sqlite3.Connection:
    database = sqlite3.connect(path)
    database.execute(
        """
        CREATE TABLE IF NOT EXISTS selfplay_runs (
            run_id INTEGER PRIMARY KEY AUTOINCREMENT,
            created_at INTEGER NOT NULL,
            model_sha256 TEXT NOT NULL,
            engine_sha256 TEXT NOT NULL,
            book_sha256 TEXT NOT NULL,
            nodes INTEGER NOT NULL,
            pair_count INTEGER NOT NULL,
            base_seed INTEGER NOT NULL,
            max_plies INTEGER NOT NULL,
            mate_proof_time_ms INTEGER NOT NULL DEFAULT 0,
            policy_weight INTEGER NOT NULL DEFAULT 50
        )
        """
    )
    database.execute(
        """
        CREATE TABLE IF NOT EXISTS selfplay_openings (
            run_id INTEGER NOT NULL,
            pair_index INTEGER NOT NULL,
            fen TEXT NOT NULL,
            PRIMARY KEY (run_id, pair_index)
        )
        """
    )
    database.execute(
        """
        CREATE TABLE IF NOT EXISTS selfplay_games (
            run_id INTEGER NOT NULL,
            pair_index INTEGER NOT NULL,
            game_in_pair INTEGER NOT NULL,
            ai_color TEXT NOT NULL,
            result TEXT NOT NULL,
            termination TEXT NOT NULL,
            plies INTEGER NOT NULL,
            duration_ms REAL NOT NULL,
            PRIMARY KEY (run_id, pair_index, game_in_pair)
        )
        """
    )
    columns = {
        str(row[1]) for row in database.execute("PRAGMA table_info(selfplay_runs)")
    }
    if "mate_proof_time_ms" not in columns:
        database.execute(
            "ALTER TABLE selfplay_runs ADD COLUMN mate_proof_time_ms INTEGER NOT NULL DEFAULT 0"
        )
    if "policy_weight" not in columns:
        database.execute(
            "ALTER TABLE selfplay_runs ADD COLUMN policy_weight INTEGER NOT NULL DEFAULT 50"
        )
    return database


def _configure_engine(
    engine: chess.engine.SimpleEngine,
    learned_policy: bool,
    game_seed: int,
    mate_proof_time_ms: int,
    policy_weight: int,
) -> None:
    reset_seed = game_seed ^ 0x5A5A5A5A
    engine.configure({"Seed": reset_seed & 0x7FFFFFFF})
    engine.configure(
        {
            "Seed": game_seed & 0x7FFFFFFF,
            "LearnedPolicy": learned_policy,
            "PolicyLearning": False,
            "PolicyWeight": policy_weight,
            "MateProofTimeMs": mate_proof_time_ms,
        }
    )


def play_game(
    executable: Path,
    opening: chess.Board,
    ai_color: chess.Color,
    nodes: int,
    seed: int,
    max_plies: int,
    mate_proof_time_ms: int,
    policy_weight: int,
) -> PlayedGame:
    control = chess.engine.SimpleEngine.popen_uci(
        [str(executable)], timeout=30.0, cwd=str(ROOT)
    )
    learned = chess.engine.SimpleEngine.popen_uci(
        [str(executable)], timeout=30.0, cwd=str(ROOT)
    )
    board = opening.copy(stack=False)
    moves: list[chess.Move] = []
    started = time.perf_counter()
    termination = "normal"
    try:
        _configure_engine(control, False, seed, mate_proof_time_ms, policy_weight)
        _configure_engine(learned, True, seed, mate_proof_time_ms, policy_weight)
        game_token = object()
        while not board.is_game_over(claim_draw=True) and len(moves) < max_plies:
            mover_is_ai = board.turn == ai_color
            mover = learned if mover_is_ai else control
            try:
                result = mover.play(
                    board,
                    chess.engine.Limit(nodes=nodes),
                    game=game_token,
                    info=chess.engine.INFO_NONE,
                )
                if result.move is None or result.move not in board.legal_moves:
                    raise chess.engine.EngineError("A motor nem legális lépést adott.")
                board.push(result.move)
                moves.append(result.move)
            except (chess.engine.EngineError, TimeoutError) as exc:
                side = "AI" if mover_is_ai else "Control"
                termination = f"forfeit: {side} {type(exc).__name__}: {exc}"
                result_text = "0-1" if board.turn == chess.WHITE else "1-0"
                return PlayedGame(
                    result_text,
                    termination,
                    len(moves),
                    (time.perf_counter() - started) * 1000.0,
                    moves,
                )

        if board.is_game_over(claim_draw=True):
            outcome = board.outcome(claim_draw=True)
            result_text = outcome.result() if outcome else "1/2-1/2"
            termination = outcome.termination.name.lower() if outcome else "normal"
        else:
            result_text = "1/2-1/2"
            termination = "max-plies"
        return PlayedGame(
            result_text,
            termination,
            len(moves),
            (time.perf_counter() - started) * 1000.0,
            moves,
        )
    finally:
        for engine in (control, learned):
            try:
                engine.quit()
            except (chess.engine.EngineError, TimeoutError):
                try:
                    engine.close()
                except Exception:
                    pass


def _write_pgn(
    stream,
    opening: chess.Board,
    played: PlayedGame,
    run_id: int,
    pair_index: int,
    game_in_pair: int,
    ai_color: chess.Color,
) -> None:
    game = chess.pgn.Game()
    game.setup(opening)
    game.headers["Event"] = f"YaC policy self-play run {run_id}"
    game.headers["Round"] = f"{pair_index + 1}.{game_in_pair + 1}"
    game.headers["White"] = "YaC-AI" if ai_color == chess.WHITE else "YaC-Control"
    game.headers["Black"] = "YaC-AI" if ai_color == chess.BLACK else "YaC-Control"
    game.headers["Result"] = played.result
    game.headers["Termination"] = played.termination
    node = game
    for move in played.moves:
        node = node.add_variation(move)
    print(game, file=stream, end="\n\n")
    stream.flush()


def start_or_resume_run(
    database: sqlite3.Connection,
    executable: Path,
    model_path: Path,
    book_path: Path,
    pair_count: int,
    nodes: int,
    base_seed: int,
    max_plies: int,
    mate_proof_time_ms: int,
    policy_weight: int,
    resume_run_id: int | None,
) -> tuple[int, list[tuple[int, chess.Board]], int, int, int, int, int]:
    model_hash = hashlib.sha256(model_path.read_bytes()).hexdigest()
    engine_hash = hashlib.sha256(executable.read_bytes()).hexdigest()
    book_hash = hashlib.sha256(book_path.read_bytes()).hexdigest()
    if resume_run_id is not None:
        metadata = database.execute(
            "SELECT model_sha256, engine_sha256, book_sha256, nodes, pair_count, base_seed, max_plies, mate_proof_time_ms, policy_weight "
            "FROM selfplay_runs WHERE run_id = ?",
            (resume_run_id,),
        ).fetchone()
        if not metadata:
            raise ValueError(f"Nincs ilyen önjátszó futás: {resume_run_id}")
        if tuple(metadata[:3]) != (model_hash, engine_hash, book_hash):
            raise ValueError("A motor, modell vagy megnyitáskönyv megváltozott; nem folytatható a futás.")
        nodes, pair_count, base_seed, max_plies, mate_proof_time_ms, policy_weight = map(int, metadata[3:])
        run_id = resume_run_id
    else:
        cursor = database.execute(
            "INSERT INTO selfplay_runs(created_at, model_sha256, engine_sha256, book_sha256, nodes, pair_count, base_seed, max_plies, mate_proof_time_ms, policy_weight) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (int(time.time()), model_hash, engine_hash, book_hash, nodes, pair_count, base_seed, max_plies, mate_proof_time_ms, policy_weight),
        )
        run_id = int(cursor.lastrowid)
        all_openings = load_openings(book_path)
        if pair_count > len(all_openings):
            raise ValueError(
                f"Csak {len(all_openings)} megnyitás érhető el, de {pair_count} pár lett kérve."
            )
        chooser = random.Random(base_seed)
        selected = chooser.sample(all_openings, pair_count)
        database.executemany(
            "INSERT INTO selfplay_openings VALUES (?, ?, ?)",
            ((run_id, index, board.fen()) for index, board in enumerate(selected)),
        )
        database.commit()
    openings = [
        (int(index), chess.Board(fen))
        for index, fen in database.execute(
            "SELECT pair_index, fen FROM selfplay_openings WHERE run_id = ? ORDER BY pair_index",
            (run_id,),
        )
    ]
    return run_id, openings, nodes, base_seed, max_plies, mate_proof_time_ms, policy_weight


def run_selfplay(
    executable: Path,
    model_path: Path,
    book_path: Path,
    database_path: Path,
    pgn_path: Path,
    pair_count: int,
    nodes: int,
    base_seed: int,
    max_plies: int,
    mate_proof_time_ms: int,
    policy_weight: int,
    resume_run_id: int | None = None,
    max_new_games: int | None = None,
) -> int:
    if not model_path.is_file():
        raise ValueError(f"A tanított modell nem található: {model_path}")
    database = _open_database(database_path)
    try:
        run_id, openings, nodes, base_seed, max_plies, mate_proof_time_ms, policy_weight = start_or_resume_run(
            database,
            executable,
            model_path,
            book_path,
            pair_count,
            nodes,
            base_seed,
            max_plies,
            mate_proof_time_ms,
            policy_weight,
            resume_run_id,
        )
        completed = {
            (int(pair), int(game))
            for pair, game in database.execute(
                "SELECT pair_index, game_in_pair FROM selfplay_games WHERE run_id = ?",
                (run_id,),
            )
        }
        total = len(openings) * 2
        new_games = 0
        pgn_path.parent.mkdir(parents=True, exist_ok=True)
        with pgn_path.open("a", encoding="utf-8") as pgn:
            for pair_index, opening in openings:
                for game_in_pair, ai_color in enumerate((chess.WHITE, chess.BLACK)):
                    if (pair_index, game_in_pair) in completed:
                        continue
                    if max_new_games is not None and new_games >= max_new_games:
                        return run_id
                    game_seed = (base_seed + pair_index * 104729) & 0x7FFFFFFF
                    played = play_game(
                        executable, opening, ai_color, nodes, game_seed, max_plies,
                        mate_proof_time_ms, policy_weight,
                    )
                    database.execute(
                        "INSERT INTO selfplay_games VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                        (
                            run_id,
                            pair_index,
                            game_in_pair,
                            "white" if ai_color == chess.WHITE else "black",
                            played.result,
                            played.termination,
                            played.plies,
                            played.duration_ms,
                        ),
                    )
                    database.commit()
                    _write_pgn(
                        pgn, opening, played, run_id, pair_index, game_in_pair, ai_color
                    )
                    completed.add((pair_index, game_in_pair))
                    new_games += 1
                    print(
                        f"Játszma {len(completed)}/{total}: {played.result}, "
                        f"AI={'világos' if ai_color else 'sötét'}, {played.plies} fél-lépés",
                        flush=True,
                    )
        return run_id
    finally:
        database.close()


def _ai_score(result: str, ai_color: str) -> float:
    if result == "1/2-1/2":
        return 0.5
    ai_won = (result == "1-0" and ai_color == "white") or (
        result == "0-1" and ai_color == "black"
    )
    return 1.0 if ai_won else 0.0


def _elo(score: float) -> float:
    score = min(1.0 - 1e-9, max(1e-9, score))
    return 400.0 * math.log10(score / (1.0 - score))


def _exact_sign_test(wins: int, losses: int) -> float:
    decisive = wins + losses
    if decisive == 0:
        return 1.0
    tail = sum(math.comb(decisive, value) for value in range(min(wins, losses) + 1))
    return min(1.0, 2.0 * tail / (2.0**decisive))


def selfplay_report(
    database_path: Path,
    run_id: int | None = None,
    bootstrap_samples: int = 10000,
) -> tuple[int, dict, str]:
    database = _open_database(database_path)
    try:
        if run_id is None:
            row = database.execute("SELECT MAX(run_id) FROM selfplay_runs").fetchone()
            run_id = int(row[0]) if row and row[0] is not None else 0
        if not run_id:
            raise ValueError("Még nincs önjátszó futás.")
        rows = database.execute(
            "SELECT pair_index, game_in_pair, ai_color, result, termination, plies, duration_ms "
            "FROM selfplay_games WHERE run_id = ? ORDER BY pair_index, game_in_pair",
            (run_id,),
        ).fetchall()
        metadata = database.execute(
            "SELECT nodes, pair_count, mate_proof_time_ms, policy_weight FROM selfplay_runs WHERE run_id = ?", (run_id,)
        ).fetchone()
    finally:
        database.close()
    if not rows:
        raise ValueError("A futásban még nincs befejezett játszma.")

    scores = [_ai_score(row[3], row[2]) for row in rows]
    wins = sum(score == 1.0 for score in scores)
    draws = sum(score == 0.5 for score in scores)
    losses = sum(score == 0.0 for score in scores)
    score = sum(scores) / len(scores)
    elo = _elo(score)
    non_forfeit_scores = [
        value for row, value in zip(rows, scores) if not str(row[4]).startswith("forfeit")
    ]
    non_forfeit_wins = sum(value == 1.0 for value in non_forfeit_scores)
    non_forfeit_draws = sum(value == 0.5 for value in non_forfeit_scores)
    non_forfeit_losses = sum(value == 0.0 for value in non_forfeit_scores)
    non_forfeit_score = (
        sum(non_forfeit_scores) / len(non_forfeit_scores) if non_forfeit_scores else 0.5
    )
    pairs: dict[int, list[float]] = {}
    for row, value in zip(rows, scores):
        pairs.setdefault(int(row[0]), []).append(value)
    complete_pairs = [values for values in pairs.values() if len(values) == 2]
    low = high = float("nan")
    if complete_pairs:
        rng = random.Random(0x5E1F)
        bootstrapped: list[float] = []
        for _ in range(bootstrap_samples):
            sample = [rng.choice(complete_pairs) for _ in complete_pairs]
            sample_score = sum(sum(pair) for pair in sample) / (2.0 * len(sample))
            bootstrapped.append(_elo(sample_score))
        bootstrapped.sort()
        low = bootstrapped[int(0.025 * (len(bootstrapped) - 1))]
        high = bootstrapped[int(0.975 * (len(bootstrapped) - 1))]

    summary = {
        "runId": run_id,
        "nodes": int(metadata[0]),
        "plannedGames": int(metadata[1]) * 2,
        "mateProofTimeMs": int(metadata[2]),
        "policyWeight": int(metadata[3]),
        "games": len(rows),
        "completePairs": len(complete_pairs),
        "wins": wins,
        "draws": draws,
        "losses": losses,
        "score": score,
        "eloDifference": elo,
        "eloCiLow": low,
        "eloCiHigh": high,
        "signTestP": _exact_sign_test(wins, losses),
        "averagePlies": sum(int(row[5]) for row in rows) / len(rows),
        "averageGameSeconds": sum(float(row[6]) for row in rows) / (1000.0 * len(rows)),
        "forfeits": sum(str(row[4]).startswith("forfeit") for row in rows),
        "aiForfeits": sum(str(row[4]).startswith("forfeit: AI") for row in rows),
        "controlForfeits": sum(str(row[4]).startswith("forfeit: Control") for row in rows),
        "nonForfeitGames": len(non_forfeit_scores),
        "nonForfeitWins": non_forfeit_wins,
        "nonForfeitDraws": non_forfeit_draws,
        "nonForfeitLosses": non_forfeit_losses,
        "nonForfeitScore": non_forfeit_score,
        "nonForfeitEloDifference": _elo(non_forfeit_score),
    }
    ci = "n/a" if math.isnan(low) else f"[{low:+.1f}, {high:+.1f}]"
    markdown = "\n".join(
        [
            f"# YaC AI–Control önjátszás – futás {run_id}",
            "",
            f"- Keret: {summary['nodes']:,} csomópont/lépés",
            f"- Mattbizonyítási időkorlát: {summary['mateProofTimeMs']:,} ms/lépés",
            f"- Tanult policy súlya: {summary['policyWeight'] / 100.0:.2f}",
            f"- Játszmák: {summary['games']}/{summary['plannedGames']} ({summary['completePairs']} teljes színcserés pár)",
            f"- AI eredmény: **{wins} győzelem / {draws} döntetlen / {losses} vereség**",
            f"- Pontarány: **{100.0*score:.1f}%**",
            f"- Becsült relatív Elo: **{elo:+.1f}**, páros bootstrap 95% CI: **{ci}**",
            f"- Győzelem/vereség egzakt p-érték: **{summary['signTestP']:.4f}**",
            f"- Átlagos játszmahossz: {summary['averagePlies']:.1f} fél-lépés",
            f"- Átlagos futásidő: {summary['averageGameSeconds']:.1f} s/játszma",
            f"- Technikai vereség: {summary['forfeits']} (AI: {summary['aiForfeits']}, kontroll: {summary['controlForfeits']})",
            f"- Csak táblán befejeződött játszmák (diagnosztika): "
            f"**{non_forfeit_wins}/{non_forfeit_draws}/{non_forfeit_losses}**, "
            f"{100.0*non_forfeit_score:.1f}%, {_elo(non_forfeit_score):+.1f} Elo",
            "",
        ]
    )
    return run_id, summary, markdown


def write_report(database_path: Path, run_id: int | None, output: Path) -> None:
    _, summary, markdown = selfplay_report(database_path, run_id)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(markdown, encoding="utf-8")
    with output.with_suffix(".csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary.keys()))
        writer.writeheader()
        writer.writerow(summary)
    print(markdown)
    print(f"Riport: {output} és {output.with_suffix('.csv')}")


def main() -> None:
    parser = argparse.ArgumentParser(description="YaC-AI kontra YaC-Control önjátszó benchmark")
    subparsers = parser.add_subparsers(dest="command", required=True)

    run = subparsers.add_parser("run")
    run.add_argument("--engine")
    run.add_argument("--model", type=Path, default=ROOT / "yac_policy.model")
    run.add_argument("--book", type=Path)
    run.add_argument("--database", type=Path, default=DEFAULT_DATABASE)
    run.add_argument("--pgn", type=Path, default=DEFAULT_DIR / "games.pgn")
    run.add_argument("--pairs", type=int, default=100)
    run.add_argument("--nodes", type=int, default=2000)
    run.add_argument("--seed", type=int, default=20260820)
    run.add_argument("--max-plies", type=int, default=200)
    run.add_argument("--mate-proof-ms", type=int, default=500)
    run.add_argument("--policy-weight", type=int, default=50)
    run.add_argument("--resume-run", type=int)
    run.add_argument("--max-games", type=int)
    run.add_argument("--output", type=Path, default=DEFAULT_DIR / "report.md")

    report = subparsers.add_parser("report")
    report.add_argument("--database", type=Path, default=DEFAULT_DATABASE)
    report.add_argument("--run-id", type=int)
    report.add_argument("--output", type=Path, default=DEFAULT_DIR / "report.md")
    args = parser.parse_args()

    if args.command == "run":
        book = args.book.resolve() if args.book else ensure_opening_book()
        run_id = run_selfplay(
            find_engine(args.engine),
            args.model.resolve(),
            book,
            args.database.resolve(),
            args.pgn.resolve(),
            max(1, args.pairs),
            max(1, args.nodes),
            args.seed,
            max(20, args.max_plies),
            min(30000, max(50, args.mate_proof_ms)),
            min(100, max(0, args.policy_weight)),
            args.resume_run,
            max(1, args.max_games) if args.max_games is not None else None,
        )
        write_report(args.database.resolve(), run_id, args.output.resolve())
    else:
        write_report(args.database.resolve(), args.run_id, args.output.resolve())


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, sqlite3.Error, zipfile.BadZipFile) as error:
        print(f"Hiba: {error}", file=sys.stderr)
        raise SystemExit(1)
