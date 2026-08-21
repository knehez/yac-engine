#!/usr/bin/env python3
"""Stream Lichess puzzle labels into the YaC learned MCTS policy."""

from __future__ import annotations

import argparse
import contextlib
import csv
import hashlib
import io
import sqlite3
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, TextIO
from urllib.parse import urlparse
from urllib.request import urlopen

try:
    from .server import ROOT, UciEngine, find_engine
except ImportError:  # Direct execution: python gui/lichess_import.py
    from server import ROOT, UciEngine, find_engine


@dataclass(frozen=True)
class ImportOptions:
    minimum_popularity: int = 80
    minimum_plays: int = 100
    maximum_rating_deviation: int = 100
    maximum_samples: int = 2000
    required_themes: frozenset[str] = frozenset()
    skip_mate_in_one: bool = True
    confidence: float = 1.0


@dataclass
class ImportStats:
    rows_read: int = 0
    rows_filtered: int = 0
    malformed_rows: int = 0
    duplicate_samples: int = 0
    illegal_samples: int = 0
    imported_samples: int = 0
    heldout_excluded: int = 0


@contextlib.contextmanager
def open_puzzle_csv(source_location: str | Path) -> Iterator[TextIO]:
    location = str(source_location)
    parsed = urlparse(location)
    remote = parsed.scheme in ("http", "https")
    suffix = Path(parsed.path if remote else location).suffix.lower()

    if suffix != ".zst":
        if remote:
            with urlopen(location) as response:
                with io.TextIOWrapper(response, encoding="utf-8-sig", newline="") as source:
                    yield source
        else:
            with Path(location).open("r", encoding="utf-8-sig", newline="") as source:
                yield source
        return

    if not remote:
        compressed_context = Path(location).open("rb")
    else:
        compressed_context = urlopen(location)

    with compressed_context as compressed:
        try:
            import zstandard  # type: ignore[import-not-found]
        except ImportError as exc:
            raise RuntimeError(
                "A .zst fájlhoz telepítsd a zstandard csomagot: "
                "python -m pip install zstandard"
            ) from exc

        reader = zstandard.ZstdDecompressor().stream_reader(compressed)
        with contextlib.closing(reader):
            with io.TextIOWrapper(reader, encoding="utf-8-sig", newline="") as source:
                yield source


def _number(row: dict[str, str], field: str) -> int:
    return int(row.get(field, "").strip())


def row_passes_filters(row: dict[str, str], options: ImportOptions) -> bool:
    themes = frozenset(row.get("Themes", "").split())
    if _number(row, "Popularity") < options.minimum_popularity:
        return False
    if _number(row, "NbPlays") < options.minimum_plays:
        return False
    if _number(row, "RatingDeviation") > options.maximum_rating_deviation:
        return False
    if options.required_themes and not options.required_themes.issubset(themes):
        return False
    if options.skip_mate_in_one and "mateIn1" in themes:
        return False
    return True


def solution_samples(row: dict[str, str]) -> Iterator[tuple[list[str], str]]:
    """Yield (prefix, target) pairs; move zero creates the shown puzzle."""
    moves = row.get("Moves", "").split()
    for target_index in range(1, len(moves), 2):
        yield moves[:target_index], moves[target_index]


def _sample_key(fen: str, best_move: str) -> str:
    material = f"{fen}|{best_move}".encode("utf-8")
    return hashlib.sha256(material).hexdigest()


def _open_index(path: Path) -> sqlite3.Connection:
    connection = sqlite3.connect(path)
    connection.execute(
        """
        CREATE TABLE IF NOT EXISTS imported_samples (
            sample_key TEXT PRIMARY KEY,
            puzzle_id TEXT NOT NULL,
            fen TEXT NOT NULL,
            best_move TEXT NOT NULL,
            imported_at INTEGER NOT NULL
        )
        """
    )
    return connection


def import_puzzles(
    engine: UciEngine,
    source_path: str | Path,
    index_path: Path,
    options: ImportOptions,
    exclusion_database: Path | None = None,
) -> ImportStats:
    stats = ImportStats()
    excluded_puzzles: set[str] = set()
    excluded_fens: set[str] = set()
    if exclusion_database and exclusion_database.is_file():
        exclusion = sqlite3.connect(exclusion_database)
        try:
            excluded_puzzles = {
                row[0] for row in exclusion.execute("SELECT puzzle_id FROM heldout_samples")
            }
            excluded_fens = {
                row[0] for row in exclusion.execute("SELECT fen FROM heldout_samples")
            }
        finally:
            exclusion.close()
    connection = _open_index(index_path)
    engine.begin_policy_import()
    try:
        with open_puzzle_csv(source_path) as source:
            reader = csv.DictReader(source)
            required = {"PuzzleId", "FEN", "Moves", "RatingDeviation", "Popularity", "NbPlays", "Themes"}
            if not reader.fieldnames or not required.issubset(reader.fieldnames):
                raise ValueError("A fájl nem Lichess puzzle CSV, vagy hiányos a fejléc.")

            for row in reader:
                if stats.imported_samples >= options.maximum_samples:
                    break
                stats.rows_read += 1
                try:
                    if row.get("PuzzleId", "").strip() in excluded_puzzles:
                        stats.heldout_excluded += 1
                        continue
                    if not row_passes_filters(row, options):
                        stats.rows_filtered += 1
                        continue
                    fen = row["FEN"].strip()
                    pairs = list(solution_samples(row))
                    if not fen or not pairs:
                        raise ValueError("Hiányzó FEN vagy megoldás")
                except (KeyError, TypeError, ValueError):
                    stats.malformed_rows += 1
                    continue

                for prefix, target in pairs:
                    if stats.imported_samples >= options.maximum_samples:
                        break
                    try:
                        position = engine.state(fen, prefix)
                    except (RuntimeError, TimeoutError, ValueError):
                        stats.illegal_samples += 1
                        break
                    if target not in position["legalMoves"]:
                        stats.illegal_samples += 1
                        break

                    canonical_fen = position["fen"]
                    if canonical_fen in excluded_fens:
                        stats.heldout_excluded += 1
                        continue
                    key = _sample_key(canonical_fen, target)
                    exists = connection.execute(
                        "SELECT 1 FROM imported_samples WHERE sample_key = ?", (key,)
                    ).fetchone()
                    if exists:
                        stats.duplicate_samples += 1
                        continue
                    if not engine.add_policy_label(canonical_fen, target, options.confidence):
                        stats.illegal_samples += 1
                        continue

                    connection.execute(
                        "INSERT INTO imported_samples VALUES (?, ?, ?, ?, ?)",
                        (key, row["PuzzleId"], canonical_fen, target, int(time.time())),
                    )
                    stats.imported_samples += 1
                    if stats.imported_samples % 100 == 0:
                        connection.commit()
                        print(
                            f"{stats.imported_samples} minta importálva "
                            f"({stats.rows_read} feldolgozott feladvány)",
                            flush=True,
                        )
        connection.commit()
    finally:
        try:
            engine.end_policy_import()
        finally:
            connection.close()
    return stats


def main() -> None:
    parser = argparse.ArgumentParser(description="Lichess puzzle minták importálása a YaC policy-ba")
    parser.add_argument("source", help="Helyi CSV/.zst fájl vagy HTTPS URL")
    parser.add_argument("--engine", help="A YaC-Engine futtatható állományának útvonala")
    parser.add_argument("--index", type=Path, default=ROOT / "yac_lichess_import.sqlite3")
    parser.add_argument("--max-samples", type=int, default=2000)
    parser.add_argument("--min-popularity", type=int, default=80)
    parser.add_argument("--min-plays", type=int, default=100)
    parser.add_argument("--max-rating-deviation", type=int, default=100)
    parser.add_argument("--themes", default="", help="Vesszővel elválasztott kötelező témák")
    parser.add_argument("--include-mate-in-one", action="store_true")
    parser.add_argument("--benchmark-database", type=Path, default=ROOT / "yac_policy_benchmark.sqlite3")
    parser.add_argument("--allow-heldout", action="store_true", help="A vakteszt állásainak importálása is")
    args = parser.parse_args()

    source = args.source
    if urlparse(source).scheme not in ("http", "https"):
        source_path = Path(source).resolve()
        if not source_path.is_file():
            parser.error(f"A forrásfájl nem található: {source_path}")
        source = source_path
    options = ImportOptions(
        minimum_popularity=args.min_popularity,
        minimum_plays=args.min_plays,
        maximum_rating_deviation=args.max_rating_deviation,
        maximum_samples=max(1, args.max_samples),
        required_themes=frozenset(filter(None, (item.strip() for item in args.themes.split(",")))),
        skip_mate_in_one=not args.include_mate_in_one,
    )

    engine = UciEngine(find_engine(args.engine))
    try:
        stats = import_puzzles(
            engine,
            source,
            args.index.resolve(),
            options,
            None if args.allow_heldout else args.benchmark_database.resolve(),
        )
    finally:
        engine.close()
    print(
        "Kész: "
        f"{stats.imported_samples} új minta, "
        f"{stats.duplicate_samples} duplikáció, "
        f"{stats.illegal_samples} illegális, "
        f"{stats.rows_filtered} kiszűrt és {stats.heldout_excluded} held-out sor."
    )


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, csv.Error) as error:
        print(f"Hiba: {error}", file=sys.stderr)
        raise SystemExit(1)
