from __future__ import annotations

import sqlite3
import tempfile
import unittest
from pathlib import Path

from gui.lichess_import import ImportOptions
from gui.policy_benchmark import benchmark_report, collect_heldout, run_benchmark


HEADER = (
    "PuzzleId,FEN,Moves,Rating,RatingDeviation,Popularity,NbPlays,Themes,"
    "GameUrl,OpeningTags,DailyDate\n"
)
SOURCE_FEN = "q3k1nr/1pp1nQpp/3p4/1P2p3/4P3/B1PP1b2/B5PP/5K2 b k - 0 17"


class FakeBenchmarkEngine:
    def __init__(self) -> None:
        self.seeds: list[int] = []
        self.target_by_fen: dict[str, str] = {}

    def state(self, fen: str, moves: list[str] | None = None) -> dict:
        canonical = f"{fen.split()[0]} w - - 0 1"
        self.target_by_fen[canonical] = "a2e6"
        return {"fen": canonical, "legalMoves": ["a2e6", "a3a4"]}

    def policy_status(self) -> dict:
        return {"trained": True}

    def set_seed(self, seed: int) -> None:
        self.seeds.append(seed)

    def analyze(
        self,
        fen: str,
        iterations: int,
        learned_policy: bool = False,
        policy_learning: bool = False,
    ) -> dict:
        target = self.target_by_fen.get(fen, "a2e6")
        best = target if learned_policy else "a3a4"
        alternatives = (
            [{"move": target, "visits": 100}, {"move": "a3a4", "visits": 10}]
            if learned_policy
            else [
                {"move": "a3a4", "visits": 100},
                {"move": "b2b3", "visits": 20},
                {"move": "c2c3", "visits": 10},
                {"move": target, "visits": 5},
            ]
        )
        return {
            "bestMove": best,
            "alternatives": alternatives,
            "nodes": iterations,
            "scoreCp": 100 if learned_policy else 0,
        }


class PolicyBenchmarkTest(unittest.TestCase):
    def test_heldout_collection_excludes_training_and_benchmark_is_paired(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            training_index = root / "training.sqlite3"
            training = sqlite3.connect(training_index)
            training.execute(
                "CREATE TABLE imported_samples "
                "(sample_key TEXT, puzzle_id TEXT, fen TEXT, best_move TEXT, imported_at INTEGER)"
            )
            training.execute(
                "INSERT INTO imported_samples VALUES ('key', 'train', 'different w - - 0 1', 'a2e6', 0)"
            )
            training.commit()
            training.close()

            source = root / "puzzles.csv"
            source.write_text(
                HEADER
                + f"train,{SOURCE_FEN},e8d7 a2e6,1700,80,90,200,mate mateIn2,,,\n"
                + f"test,{SOURCE_FEN},e8d7 a2e6,1750,75,95,300,mate mateIn2,,,\n",
                encoding="utf-8",
            )
            database = root / "benchmark.sqlite3"
            model = root / "yac_policy.model"
            model.write_text("trained model", encoding="utf-8")
            engine = FakeBenchmarkEngine()

            collected = collect_heldout(
                engine, source, database, training_index, 1, ImportOptions()
            )
            self.assertEqual(collected.collected, 1)
            self.assertEqual(collected.training_overlap, 1)

            run_id = run_benchmark(engine, database, [2000], 1, 42, model)
            _, summaries, markdown = benchmark_report(database, run_id)
            self.assertEqual(summaries[0]["controlTop1"], 0.0)
            self.assertEqual(summaries[0]["learnedTop1"], 1.0)
            self.assertEqual(summaries[0]["improved"], 1)
            self.assertEqual(summaries[0]["regressed"], 0)
            self.assertEqual(engine.seeds[0], engine.seeds[1])
            self.assertIn("+100.0 pp", markdown)

            seed_count = len(engine.seeds)
            resumed = run_benchmark(engine, database, [9999], 99, 999, model, run_id)
            self.assertEqual(resumed, run_id)
            self.assertEqual(len(engine.seeds), seed_count)


if __name__ == "__main__":
    unittest.main()
