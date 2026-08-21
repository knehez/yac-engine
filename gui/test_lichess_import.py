from __future__ import annotations

import tempfile
import unittest
import sqlite3
from pathlib import Path

from gui.lichess_import import ImportOptions, import_puzzles, solution_samples


HEADER = (
    "PuzzleId,FEN,Moves,Rating,RatingDeviation,Popularity,NbPlays,Themes,"
    "GameUrl,OpeningTags,DailyDate\n"
)
FEN = "q3k1nr/1pp1nQpp/3p4/1P2p3/4P3/B1PP1b2/B5PP/5K2 b k - 0 17"


class FakeEngine:
    def __init__(self) -> None:
        self.labels: list[tuple[str, str, float]] = []
        self.batches = 0

    def begin_policy_import(self) -> None:
        self.batches += 1

    def end_policy_import(self) -> dict:
        self.batches -= 1
        return {"accepted": len(self.labels), "saved": True}

    def state(self, fen: str, moves: list[str] | None = None) -> dict:
        prefix = moves or []
        canonical = f"{fen.split()[0]} {'w' if len(prefix) % 2 else 'b'} - - 0 1"
        target = "a2e6" if len(prefix) == 1 else "f7f8"
        return {"fen": canonical, "legalMoves": [target]}

    def add_policy_label(self, fen: str, best_move: str, confidence: float) -> bool:
        self.labels.append((fen, best_move, confidence))
        return True


class LichessImportTest(unittest.TestCase):
    def test_solution_starts_after_opponents_first_move(self) -> None:
        row = {"Moves": "e8d7 a2e6 d7d8 f7f8"}
        self.assertEqual(
            list(solution_samples(row)),
            [(["e8d7"], "a2e6"), (["e8d7", "a2e6", "d7d8"], "f7f8")],
        )

    def test_import_filters_labels_and_deduplicates(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "puzzles.csv"
            source.write_text(
                HEADER
                + f"good,{FEN},e8d7 a2e6 d7d8 f7f8,1760,80,90,150,mate mateIn2 middlegame,,,\n"
                + f"weak,{FEN},e8d7 a2e6,1760,80,20,150,mate mateIn2 middlegame,,,\n",
                encoding="utf-8",
            )
            index = root / "index.sqlite3"
            options = ImportOptions(maximum_samples=10)

            first_engine = FakeEngine()
            first = import_puzzles(first_engine, source, index, options)
            self.assertEqual(first.imported_samples, 2)
            self.assertEqual(first.rows_filtered, 1)
            self.assertEqual([label[1] for label in first_engine.labels], ["a2e6", "f7f8"])
            self.assertEqual(first_engine.batches, 0)

            second_engine = FakeEngine()
            second = import_puzzles(second_engine, source, index, options)
            self.assertEqual(second.imported_samples, 0)
            self.assertEqual(second.duplicate_samples, 2)
            self.assertEqual(second_engine.labels, [])
            self.assertEqual(second_engine.batches, 0)

    def test_import_never_uses_heldout_puzzle(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "puzzles.csv"
            source.write_text(
                HEADER
                + f"heldout,{FEN},e8d7 a2e6,1760,80,90,150,mate mateIn2 middlegame,,,\n",
                encoding="utf-8",
            )
            exclusion_path = root / "benchmark.sqlite3"
            exclusion = sqlite3.connect(exclusion_path)
            exclusion.execute("CREATE TABLE heldout_samples (puzzle_id TEXT, fen TEXT)")
            exclusion.execute("INSERT INTO heldout_samples VALUES ('heldout', 'unused w - - 0 1')")
            exclusion.commit()
            exclusion.close()

            engine = FakeEngine()
            result = import_puzzles(
                engine,
                source,
                root / "index.sqlite3",
                ImportOptions(maximum_samples=10),
                exclusion_path,
            )
            self.assertEqual(result.heldout_excluded, 1)
            self.assertEqual(result.imported_samples, 0)
            self.assertEqual(engine.labels, [])


if __name__ == "__main__":
    unittest.main()
