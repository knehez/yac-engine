#!/usr/bin/env python3
"""Held-out Lichess benchmark for the learned YaC MCTS policy."""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import sqlite3
import sys
import time
from dataclasses import dataclass
from pathlib import Path

try:
    from .lichess_import import ImportOptions, open_puzzle_csv, row_passes_filters, solution_samples
    from .server import ROOT, UciEngine, find_engine
except ImportError:  # Direct execution: python gui/policy_benchmark.py
    from lichess_import import ImportOptions, open_puzzle_csv, row_passes_filters, solution_samples
    from server import ROOT, UciEngine, find_engine


DEFAULT_SOURCE = "https://database.lichess.org/lichess_db_puzzle.csv.zst"
DEFAULT_DATABASE = ROOT / "yac_policy_benchmark.sqlite3"
DEFAULT_TRAINING_INDEX = ROOT / "yac_lichess_import.sqlite3"


@dataclass
class CollectionStats:
    rows_read: int = 0
    filtered: int = 0
    training_overlap: int = 0
    duplicate: int = 0
    illegal: int = 0
    collected: int = 0


def _sample_key(fen: str, best_move: str) -> str:
    return hashlib.sha256(f"{fen}|{best_move}".encode("utf-8")).hexdigest()


def _open_database(path: Path) -> sqlite3.Connection:
    connection = sqlite3.connect(path)
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS heldout_samples (
            sample_key TEXT PRIMARY KEY,
            puzzle_id TEXT NOT NULL UNIQUE,
            fen TEXT NOT NULL,
            best_move TEXT NOT NULL,
            rating INTEGER NOT NULL,
            popularity INTEGER NOT NULL,
            themes TEXT NOT NULL
        )
        """
    )
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS benchmark_runs (
            run_id INTEGER PRIMARY KEY AUTOINCREMENT,
            created_at INTEGER NOT NULL,
            model_sha256 TEXT NOT NULL,
            sample_limit INTEGER NOT NULL,
            budgets TEXT NOT NULL,
            base_seed INTEGER NOT NULL
        )
        """
    )
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS benchmark_results (
            run_id INTEGER NOT NULL,
            sample_key TEXT NOT NULL,
            iterations INTEGER NOT NULL,
            seed INTEGER NOT NULL,
            control_best TEXT NOT NULL,
            learned_best TEXT NOT NULL,
            control_top3 INTEGER NOT NULL,
            learned_top3 INTEGER NOT NULL,
            control_target_visits INTEGER NOT NULL,
            learned_target_visits INTEGER NOT NULL,
            control_nodes INTEGER NOT NULL,
            learned_nodes INTEGER NOT NULL,
            control_score_cp INTEGER NOT NULL,
            learned_score_cp INTEGER NOT NULL,
            control_elapsed_ms REAL NOT NULL,
            learned_elapsed_ms REAL NOT NULL,
            PRIMARY KEY (run_id, sample_key, iterations),
            FOREIGN KEY (sample_key) REFERENCES heldout_samples(sample_key)
        )
        """
    )
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS benchmark_run_samples (
            run_id INTEGER NOT NULL,
            sample_key TEXT NOT NULL,
            ordinal INTEGER NOT NULL,
            PRIMARY KEY (run_id, sample_key),
            UNIQUE (run_id, ordinal)
        )
        """
    )
    return connection


def collect_heldout(
    engine: UciEngine,
    source: str | Path,
    database_path: Path,
    training_index_path: Path,
    count: int,
    options: ImportOptions,
) -> CollectionStats:
    if not training_index_path.is_file():
        raise ValueError(f"A tanítóindex nem található: {training_index_path}")
    training = sqlite3.connect(training_index_path)
    try:
        training_puzzles = {
            row[0] for row in training.execute("SELECT DISTINCT puzzle_id FROM imported_samples")
        }
        training_fens = {
            row[0] for row in training.execute("SELECT DISTINCT fen FROM imported_samples")
        }
    finally:
        training.close()

    database = _open_database(database_path)
    stats = CollectionStats()
    try:
        existing_puzzles = {
            row[0] for row in database.execute("SELECT puzzle_id FROM heldout_samples")
        }
        existing_keys = {
            row[0] for row in database.execute("SELECT sample_key FROM heldout_samples")
        }
        remaining = max(0, count - len(existing_puzzles))
        if remaining == 0:
            return stats
        with open_puzzle_csv(source) as stream:
            reader = csv.DictReader(stream)
            required = {"PuzzleId", "FEN", "Moves", "Rating", "RatingDeviation", "Popularity", "NbPlays", "Themes"}
            if not reader.fieldnames or not required.issubset(reader.fieldnames):
                raise ValueError("A fájl nem Lichess puzzle CSV, vagy hiányos a fejléc.")

            for row in reader:
                if stats.collected >= remaining:
                    break
                stats.rows_read += 1
                try:
                    if not row_passes_filters(row, options):
                        stats.filtered += 1
                        continue
                    puzzle_id = row["PuzzleId"].strip()
                    if puzzle_id in training_puzzles:
                        stats.training_overlap += 1
                        continue
                    if puzzle_id in existing_puzzles:
                        stats.duplicate += 1
                        continue
                    prefix, target = next(solution_samples(row))
                    position = engine.state(row["FEN"].strip(), prefix)
                    if target not in position["legalMoves"]:
                        stats.illegal += 1
                        continue
                    canonical_fen = position["fen"]
                    if canonical_fen in training_fens:
                        stats.training_overlap += 1
                        continue
                    key = _sample_key(canonical_fen, target)
                    if key in existing_keys:
                        stats.duplicate += 1
                        continue
                    database.execute(
                        "INSERT INTO heldout_samples VALUES (?, ?, ?, ?, ?, ?, ?)",
                        (
                            key,
                            puzzle_id,
                            canonical_fen,
                            target,
                            int(row["Rating"]),
                            int(row["Popularity"]),
                            row["Themes"].strip(),
                        ),
                    )
                except (KeyError, StopIteration, TypeError, ValueError, RuntimeError, TimeoutError):
                    stats.illegal += 1
                    continue

                existing_puzzles.add(puzzle_id)
                existing_keys.add(key)
                stats.collected += 1
                if stats.collected % 100 == 0:
                    database.commit()
                    print(f"{stats.collected} held-out állás összegyűjtve", flush=True)
        database.commit()
    finally:
        database.close()
    return stats


def _target_metrics(result: dict, target: str) -> tuple[int, int]:
    best_is_target = result.get("bestMove") == target
    for rank, candidate in enumerate(result.get("alternatives", []), start=1):
        if candidate.get("move") == target:
            return int(best_is_target or rank <= 3), int(candidate.get("visits", 0))
    return int(best_is_target), 0


def _paired_seed(base_seed: int, sample_key: str, iterations: int) -> int:
    digest = hashlib.sha256(f"{base_seed}:{sample_key}:{iterations}".encode("ascii")).digest()
    return int.from_bytes(digest[:4], "little") & 0x7FFFFFFF


def run_benchmark(
    engine: UciEngine,
    database_path: Path,
    budgets: list[int],
    sample_limit: int,
    base_seed: int,
    model_path: Path,
    resume_run_id: int | None = None,
    max_new_pairs: int | None = None,
) -> int:
    status = engine.policy_status()
    if not status["trained"]:
        raise ValueError("Nincs betanított policy-modell; a vakteszthez előbb tanítsd be.")
    if not model_path.is_file():
        raise ValueError(f"A policy-modell nem található: {model_path}")
    model_hash = hashlib.sha256(model_path.read_bytes()).hexdigest()

    database = _open_database(database_path)
    try:
        if resume_run_id is not None:
            metadata = database.execute(
                "SELECT model_sha256, sample_limit, budgets, base_seed FROM benchmark_runs WHERE run_id = ?",
                (resume_run_id,),
            ).fetchone()
            if not metadata:
                raise ValueError(f"Nincs ilyen benchmark-futás: {resume_run_id}")
            if metadata[0] != model_hash:
                raise ValueError("A policy-modell megváltozott; a régi futás nem folytatható.")
            sample_limit = int(metadata[1])
            budgets = _parse_budgets(metadata[2])
            base_seed = int(metadata[3])
            run_id = resume_run_id
            samples = database.execute(
                "SELECT s.sample_key, s.puzzle_id, s.fen, s.best_move "
                "FROM benchmark_run_samples rs JOIN heldout_samples s USING(sample_key) "
                "WHERE rs.run_id = ? ORDER BY rs.ordinal",
                (run_id,),
            ).fetchall()
            if not samples:  # Compatibility with a partial run created before run snapshots.
                samples = database.execute(
                    "SELECT sample_key, puzzle_id, fen, best_move FROM heldout_samples "
                    "ORDER BY sample_key LIMIT ?",
                    (sample_limit,),
                ).fetchall()
        else:
            samples = database.execute(
                "SELECT sample_key, puzzle_id, fen, best_move FROM heldout_samples "
                "ORDER BY sample_key LIMIT ?",
                (sample_limit,),
            ).fetchall()
            if not samples:
                raise ValueError("A held-out tesztkészlet üres; előbb futtasd a collect parancsot.")
            cursor = database.execute(
                "INSERT INTO benchmark_runs(created_at, model_sha256, sample_limit, budgets, base_seed) "
                "VALUES (?, ?, ?, ?, ?)",
                (int(time.time()), model_hash, len(samples), ",".join(map(str, budgets)), base_seed),
            )
            run_id = int(cursor.lastrowid)
            database.executemany(
                "INSERT INTO benchmark_run_samples VALUES (?, ?, ?)",
                ((run_id, sample[0], ordinal) for ordinal, sample in enumerate(samples)),
            )
            database.commit()

        if not samples:
            raise ValueError("A benchmark-futáshoz nem tartozik tesztállás.")
        completed_pairs = {
            (row[0], int(row[1]))
            for row in database.execute(
                "SELECT sample_key, iterations FROM benchmark_results WHERE run_id = ?", (run_id,)
            )
        }

        completed = len(completed_pairs)
        total = len(samples) * len(budgets)
        new_pairs = 0
        for sample_key, puzzle_id, fen, target in samples:
            for iterations in budgets:
                if (sample_key, iterations) in completed_pairs:
                    continue
                if max_new_pairs is not None and new_pairs >= max_new_pairs:
                    return run_id
                seed = _paired_seed(base_seed, sample_key, iterations)

                engine.set_seed(seed)
                started = time.perf_counter()
                control = engine.analyze(
                    fen, iterations, learned_policy=False, policy_learning=False
                )
                control_elapsed = (time.perf_counter() - started) * 1000.0

                engine.set_seed(seed)
                started = time.perf_counter()
                learned = engine.analyze(
                    fen, iterations, learned_policy=True, policy_learning=False
                )
                learned_elapsed = (time.perf_counter() - started) * 1000.0

                control_top3, control_visits = _target_metrics(control, target)
                learned_top3, learned_visits = _target_metrics(learned, target)
                database.execute(
                    "INSERT INTO benchmark_results VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                    (
                        run_id,
                        sample_key,
                        iterations,
                        seed,
                        control["bestMove"],
                        learned["bestMove"],
                        control_top3,
                        learned_top3,
                        control_visits,
                        learned_visits,
                        int(control["nodes"]),
                        int(learned["nodes"]),
                        int(control["scoreCp"]),
                        int(learned["scoreCp"]),
                        control_elapsed,
                        learned_elapsed,
                    ),
                )
                database.commit()
                completed += 1
                new_pairs += 1
                if completed % 10 == 0 or completed == total:
                    print(f"Benchmark: {completed}/{total} mérési pár", flush=True)
        return run_id
    finally:
        database.close()


def _mcnemar_exact(improved: int, regressed: int) -> float:
    discordant = improved + regressed
    if discordant == 0:
        return 1.0
    tail = sum(math.comb(discordant, value) for value in range(min(improved, regressed) + 1))
    return min(1.0, 2.0 * tail / (2.0**discordant))


def benchmark_report(database_path: Path, run_id: int | None = None) -> tuple[int, list[dict], str]:
    database = _open_database(database_path)
    try:
        if run_id is None:
            row = database.execute("SELECT MAX(run_id) FROM benchmark_runs").fetchone()
            run_id = int(row[0]) if row and row[0] is not None else 0
        if not run_id:
            raise ValueError("Még nincs benchmark-futás.")
        rows = database.execute(
            """
            SELECT r.iterations,
                   COUNT(*),
                   SUM(r.control_best = s.best_move),
                   SUM(r.learned_best = s.best_move),
                   SUM(r.control_top3 OR r.control_best = s.best_move),
                   SUM(r.learned_top3 OR r.learned_best = s.best_move),
                   SUM(r.control_best != s.best_move AND r.learned_best = s.best_move),
                   SUM(r.control_best = s.best_move AND r.learned_best != s.best_move),
                   AVG(r.control_target_visits),
                   AVG(r.learned_target_visits),
                   AVG(r.control_elapsed_ms),
                   AVG(r.learned_elapsed_ms)
            FROM benchmark_results r
            JOIN heldout_samples s USING(sample_key)
            WHERE r.run_id = ?
            GROUP BY r.iterations
            ORDER BY r.iterations
            """,
            (run_id,),
        ).fetchall()
    finally:
        database.close()

    summaries: list[dict] = []
    lines = [
        f"# YaC policy vakteszt – futás {run_id}",
        "",
        "| Iteráció | N | Top-1 nélkül | Top-1 AI | Eltérés | Top-3 nélkül | Top-3 AI | Javult/romlott | McNemar p |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for row in rows:
        iterations, count, control_hit, learned_hit, control_top3, learned_top3, improved, regressed, control_visits, learned_visits, control_ms, learned_ms = row
        delta = 100.0 * (learned_hit - control_hit) / count
        p_value = _mcnemar_exact(improved, regressed)
        summary = {
            "iterations": iterations,
            "positions": count,
            "controlTop1": control_hit / count,
            "learnedTop1": learned_hit / count,
            "deltaPercentagePoints": delta,
            "controlTop3": control_top3 / count,
            "learnedTop3": learned_top3 / count,
            "improved": improved,
            "regressed": regressed,
            "mcnemarP": p_value,
            "controlTargetVisitsAverage": control_visits,
            "learnedTargetVisitsAverage": learned_visits,
            "controlElapsedMsAverage": control_ms,
            "learnedElapsedMsAverage": learned_ms,
        }
        summaries.append(summary)
        lines.append(
            f"| {iterations:,} | {count} | {100*summary['controlTop1']:.1f}% | "
            f"{100*summary['learnedTop1']:.1f}% | {delta:+.1f} pp | "
            f"{100*summary['controlTop3']:.1f}% | {100*summary['learnedTop3']:.1f}% | "
            f"{improved}/{regressed} | {p_value:.4f} |"
        )
    return run_id, summaries, "\n".join(lines) + "\n"


def write_report(database_path: Path, run_id: int | None, output: Path) -> str:
    resolved_run_id, summaries, markdown = benchmark_report(database_path, run_id)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(markdown, encoding="utf-8")
    csv_path = output.with_suffix(".csv")
    with csv_path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summaries[0].keys()) if summaries else ["iterations"])
        writer.writeheader()
        writer.writerows(summaries)
    print(markdown)
    print(f"Riport: {output} és {csv_path}")
    return str(resolved_run_id)


def _parse_budgets(value: str) -> list[int]:
    budgets = sorted({int(item.strip()) for item in value.split(",") if item.strip()})
    if not budgets or budgets[0] < 1:
        raise argparse.ArgumentTypeError("Legalább egy pozitív iterációszám szükséges.")
    return budgets


def main() -> None:
    parser = argparse.ArgumentParser(description="YaC tanult MCTS-policy vakteszt")
    subparsers = parser.add_subparsers(dest="command", required=True)

    collect = subparsers.add_parser("collect", help="Elkülönített Lichess tesztkészlet gyűjtése")
    collect.add_argument("--source", default=DEFAULT_SOURCE)
    collect.add_argument("--database", type=Path, default=DEFAULT_DATABASE)
    collect.add_argument("--training-index", type=Path, default=DEFAULT_TRAINING_INDEX)
    collect.add_argument("--count", type=int, default=2000)
    collect.add_argument("--engine")

    run = subparsers.add_parser("run", help="Policy nélküli és policy-s párosított mérés")
    run.add_argument("--database", type=Path, default=DEFAULT_DATABASE)
    run.add_argument("--iterations", type=_parse_budgets, default=_parse_budgets("2000,10000,50000"))
    run.add_argument("--limit", type=int, default=2000)
    run.add_argument("--seed", type=int, default=20260820)
    run.add_argument("--model", type=Path, default=ROOT / "yac_policy.model")
    run.add_argument("--engine")
    run.add_argument("--output", type=Path, default=ROOT / "build" / "policy-benchmark.md")
    run.add_argument("--resume-run", type=int, help="Megszakított futás azonosítója")
    run.add_argument("--max-pairs", type=int, help="Legfeljebb ennyi új mérési pár ebben a folyamatban")

    report = subparsers.add_parser("report", help="Korábbi mérés riportjának újragenerálása")
    report.add_argument("--database", type=Path, default=DEFAULT_DATABASE)
    report.add_argument("--run-id", type=int)
    report.add_argument("--output", type=Path, default=ROOT / "build" / "policy-benchmark.md")
    args = parser.parse_args()

    if args.command == "collect":
        engine = UciEngine(find_engine(args.engine))
        try:
            stats = collect_heldout(
                engine,
                args.source,
                args.database.resolve(),
                args.training_index.resolve(),
                max(1, args.count),
                ImportOptions(),
            )
        finally:
            engine.close()
        print(
            f"Kész: {stats.collected} tesztállás, {stats.training_overlap} tanítási átfedés, "
            f"{stats.filtered} kiszűrt és {stats.illegal} hibás sor."
        )
    elif args.command == "run":
        engine = UciEngine(find_engine(args.engine))
        try:
            run_id = run_benchmark(
                engine,
                args.database.resolve(),
                args.iterations,
                max(1, args.limit),
                args.seed,
                args.model.resolve(),
                args.resume_run,
                max(1, args.max_pairs) if args.max_pairs is not None else None,
            )
        finally:
            engine.close()
        write_report(args.database.resolve(), run_id, args.output.resolve())
    else:
        write_report(args.database.resolve(), args.run_id, args.output.resolve())


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, sqlite3.Error, csv.Error) as error:
        print(f"Hiba: {error}", file=sys.stderr)
        raise SystemExit(1)
